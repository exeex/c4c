#include "bir.hpp"
#include "bir_control_flow_view.hpp"

namespace c4c::backend::bir {

struct BirControlFlowView::Implementation {
  const Function* function = nullptr;
};

namespace {
bool same_block(const Block& lhs, const Block& rhs) {
  if (lhs.label_id != kInvalidBlockLabel && rhs.label_id != kInvalidBlockLabel)
    return lhs.label_id == rhs.label_id;
  return !lhs.label.empty() && lhs.label == rhs.label;
}

bool names_block(const Terminator& term, const Block& block) {
  auto matches = [&](std::string_view name, BlockLabelId id) {
    if (id != kInvalidBlockLabel && block.label_id != kInvalidBlockLabel)
      return id == block.label_id;
    return !name.empty() && name == block.label;
  };
  if (term.kind == TerminatorKind::Branch)
    return matches(term.target_label, term.target_label_id);
  if (term.kind == TerminatorKind::CondBranch)
    return matches(term.true_label, term.true_label_id) ||
           matches(term.false_label, term.false_label_id);
  return false;
}
}  // namespace

BirControlFlowView make_bir_control_flow_view(const Function& function) {
  return BirControlFlowView(std::make_shared<BirControlFlowView::Implementation>(
      BirControlFlowView::Implementation{.function = &function}));
}

BirControlFlowResult find_selection(
    const BirControlFlowView& view, const Block& block, std::size_t number) {
  BirControlFlowResult result;
  if (!view.implementation_ || !view.implementation_->function ||
      number >= block.insts.size()) return result;
  const auto* selection = std::get_if<SelectInst>(&block.insts[number]);
  if (!selection) return result;
  result.block = &block;
  result.selection = selection;
  result.condition = &selection->lhs;
  result.true_value = &selection->true_value;
  result.false_value = &selection->false_value;
  result.instruction_number = number;
  result.complete = !selection->result.name.empty();
  result.status = result.complete ? BirViewStatus::Available : BirViewStatus::Incomplete;
  return result;
}

BirControlFlowResult find_branch_condition(
    const BirControlFlowView& view, const Block& block) {
  BirControlFlowResult result;
  if (!view.implementation_ || !view.implementation_->function ||
      block.terminator.kind != TerminatorKind::CondBranch) return result;
  result.block = &block;
  result.condition = &block.terminator.condition;
  result.complete = block.terminator.condition.kind == Value::Kind::Immediate ||
                    !block.terminator.condition.name.empty();
  result.status = result.complete ? BirViewStatus::Available : BirViewStatus::Incomplete;
  return result;
}

BirControlFlowResult find_block_relationship(
    const BirControlFlowView& view, const Block& from, const Block& to) {
  BirControlFlowResult result;
  if (!view.implementation_ || !view.implementation_->function ||
      !names_block(from.terminator, to)) return result;
  result.block = &from;
  result.related_block = &to;
  result.relationship_name = to.label;
  std::size_t matches = 0;
  for (const auto& block : view.implementation_->function->blocks)
    if (same_block(block, to)) ++matches;
  if (matches > 1) {
    result.status = BirViewStatus::Ambiguous;
    return result;
  }
  result.complete = matches == 1;
  result.status = result.complete ? BirViewStatus::Available : BirViewStatus::Incomplete;
  return result;
}

BirControlFlowResult find_join_source(
    const BirControlFlowView& view,
    const Block& block,
    std::size_t number,
    const Block& from) {
  BirControlFlowResult result;
  if (!view.implementation_ || !view.implementation_->function ||
      number >= block.insts.size()) return result;
  const auto* join = std::get_if<PhiInst>(&block.insts[number]);
  if (!join) return result;
  result.block = &block;
  result.related_block = &from;
  result.join = join;
  result.instruction_number = number;
  for (const auto& incoming : join->incomings) {
    const bool matches =
        (incoming.label_id != kInvalidBlockLabel &&
         from.label_id != kInvalidBlockLabel && incoming.label_id == from.label_id) ||
        (!incoming.label.empty() && incoming.label == from.label);
    if (!matches) continue;
    if (result.join_source) {
      result.status = BirViewStatus::Ambiguous;
      return result;
    }
    result.join_source = &incoming.value;
    result.relationship_name = incoming.label;
  }
  if (!result.join_source) {
    result.status = BirViewStatus::Incomplete;
    return result;
  }
  result.complete = result.join_source->kind == Value::Kind::Immediate ||
                    !result.join_source->name.empty();
  result.status = result.complete ? BirViewStatus::Available : BirViewStatus::Incomplete;
  return result;
}

}  // namespace c4c::backend::bir
