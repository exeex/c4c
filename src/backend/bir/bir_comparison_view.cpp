#include "bir.hpp"
#include "bir_comparison_view.hpp"

namespace c4c::backend::bir {

namespace {
bool is_comparison(BinaryOpcode opcode) {
  return opcode == BinaryOpcode::Eq || opcode == BinaryOpcode::Ne ||
         opcode == BinaryOpcode::Slt || opcode == BinaryOpcode::Sle ||
         opcode == BinaryOpcode::Sgt || opcode == BinaryOpcode::Sge ||
         opcode == BinaryOpcode::Ult || opcode == BinaryOpcode::Ule ||
         opcode == BinaryOpcode::Ugt || opcode == BinaryOpcode::Uge;
}
}  // namespace

struct BirComparisonView::Implementation {
  const Block* block = nullptr;
};

BirComparisonView make_bir_comparison_view(const Block& block) {
  return BirComparisonView(
      std::make_shared<BirComparisonView::Implementation>(
          BirComparisonView::Implementation{.block = &block}));
}

BirComparisonResult find_comparison(
    const BirComparisonView& view, std::size_t instruction_number) {
  BirComparisonResult result{};
  if (!view.implementation_ || view.implementation_->block == nullptr ||
      instruction_number >= view.implementation_->block->insts.size()) {
    return result;
  }
  const auto& block = *view.implementation_->block;
  const auto* binary = std::get_if<BinaryInst>(&block.insts[instruction_number]);
  if (binary == nullptr || !is_comparison(binary->opcode)) {
    return result;
  }
  result.comparison = binary;
  result.predicate = binary->opcode;
  result.left_operand = &binary->lhs;
  result.right_operand = &binary->rhs;
  result.materialized_condition = &binary->result;
  if (binary->result.kind != Value::Kind::Named || binary->result.name.empty()) {
    result.status = BirViewStatus::Incomplete;
    return result;
  }
  std::size_t producers = 0;
  for (const auto& instruction : block.insts) {
    const auto* candidate = std::get_if<BinaryInst>(&instruction);
    if (candidate != nullptr && is_comparison(candidate->opcode) &&
        candidate->result.kind == Value::Kind::Named &&
        candidate->result.name == binary->result.name &&
        candidate->result.type == binary->result.type) {
      ++producers;
    }
  }
  if (producers > 1U) {
    result.status = BirViewStatus::Ambiguous;
    return result;
  }
  result.used_by_branch =
      block.terminator.kind == TerminatorKind::CondBranch &&
      block.terminator.condition.kind == Value::Kind::Named &&
      block.terminator.condition.name == binary->result.name &&
      block.terminator.condition.type == binary->result.type;
  result.status = BirViewStatus::Available;
  return result;
}

}  // namespace c4c::backend::bir
