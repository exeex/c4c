#include "lir_to_bir.hpp"

#include <unordered_set>
#include <utility>

namespace c4c::backend::lir_to_bir_detail {

BlockLookup make_block_lookup(const c4c::codegen::lir::LirFunction& function) {
  BlockLookup blocks;
  for (const auto& block : function.blocks) {
    blocks.emplace(block.label, &block);
  }
  return blocks;
}

std::optional<BranchChain> follow_empty_branch_chain(const BlockLookup& blocks,
                                                     const std::string& start_label) {
  std::unordered_set<std::string> seen;
  const auto* current = [&]() -> const c4c::codegen::lir::LirBlock* {
    const auto it = blocks.find(start_label);
    return it == blocks.end() ? nullptr : it->second;
  }();
  if (current == nullptr) {
    return std::nullopt;
  }

  BranchChain chain;
  while (current != nullptr) {
    if (!seen.emplace(current->label).second || !current->insts.empty()) {
      return std::nullopt;
    }

    const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&current->terminator);
    if (br == nullptr) {
      return std::nullopt;
    }

    chain.labels.push_back(current->label);
    const auto next_it = blocks.find(br->target_label);
    if (next_it == blocks.end()) {
      return std::nullopt;
    }

    const auto* next = next_it->second;
    if (!next->insts.empty() ||
        !std::holds_alternative<c4c::codegen::lir::LirBr>(next->terminator)) {
      chain.leaf_label = current->label;
      chain.join_label = br->target_label;
      return chain;
    }

    current = next;
  }

  return std::nullopt;
}

std::optional<BranchChain> follow_canonical_select_chain(const BlockLookup& blocks,
                                                         const std::string& start_label) {
  std::unordered_set<std::string> seen;
  const auto* current = [&]() -> const c4c::codegen::lir::LirBlock* {
    const auto it = blocks.find(start_label);
    return it == blocks.end() ? nullptr : it->second;
  }();
  if (current == nullptr) {
    return std::nullopt;
  }

  BranchChain chain;
  bool saw_non_empty = false;
  while (current != nullptr) {
    if (!seen.emplace(current->label).second) {
      return std::nullopt;
    }
    if (!current->insts.empty()) {
      if (saw_non_empty) {
        return std::nullopt;
      }
      saw_non_empty = true;
    }

    const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&current->terminator);
    if (br == nullptr) {
      return std::nullopt;
    }

    chain.labels.push_back(current->label);
    const auto next_it = blocks.find(br->target_label);
    if (next_it == blocks.end()) {
      return std::nullopt;
    }

    const auto* next = next_it->second;
    const bool next_continues_chain = std::holds_alternative<c4c::codegen::lir::LirBr>(
                                          next->terminator) &&
                                      (next->insts.empty() || !saw_non_empty);
    if (!next_continues_chain) {
      chain.leaf_label = current->label;
      chain.join_label = br->target_label;
      return chain;
    }

    current = next;
  }

  return std::nullopt;
}

std::optional<PhiBlockPlanMap> collect_phi_lowering_plans(
    const c4c::codegen::lir::LirFunction& function) {
  PhiBlockPlanMap plans;

  for (const auto& block : function.blocks) {
    std::vector<PhiLoweringPlan> block_plans;
    bool saw_non_phi = false;
    for (const auto& inst : block.insts) {
      const auto* phi = std::get_if<c4c::codegen::lir::LirPhiOp>(&inst);
      if (phi == nullptr) {
        saw_non_phi = true;
        continue;
      }
      if (saw_non_phi || phi->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return std::nullopt;
      }

      const auto phi_type = lower_integer_type(phi->type_str.str());
      if (!phi_type.has_value()) {
        return std::nullopt;
      }

      PhiLoweringPlan plan{
          .result_name = phi->result.str(),
          .type = *phi_type,
      };
      for (const auto& [value, label] : phi->incoming) {
        plan.incomings.push_back({label, c4c::codegen::lir::LirOperand(value)});
      }
      if (plan.incomings.empty()) {
        return std::nullopt;
      }
      block_plans.push_back(std::move(plan));
    }

    if (!block_plans.empty()) {
      plans.emplace(block.label, std::move(block_plans));
    }
  }

  return plans;
}
}  // namespace c4c::backend::lir_to_bir_detail
