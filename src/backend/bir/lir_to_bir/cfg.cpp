#include "lowering.hpp"

#include <unordered_set>
#include <utility>

namespace c4c::backend {

BirFunctionLowerer::BlockLookup BirFunctionLowerer::make_block_lookup() const {
  // Raw LIR block spellings are a pre-BIR lowering lookup. Emitted BIR block
  // references receive BlockLabelId during module finalization.
  BlockLookup blocks;
  for (const auto& block : function_.blocks) {
    blocks.emplace(block.label, &block);
  }
  return blocks;
}

std::optional<BirFunctionLowerer::BranchChain> BirFunctionLowerer::follow_empty_branch_chain(
    const BlockLookup& blocks,
    const std::string& start_label) {
  // Selector/branch-chain recognition runs before BlockLabelId assignment, so
  // the traversal set is keyed by raw LIR labels only inside this lowering pass.
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

std::optional<BirFunctionLowerer::BranchChain> BirFunctionLowerer::follow_canonical_select_chain(
    const BlockLookup& blocks,
    const std::string& start_label) {
  // Selector recognition uses raw LIR labels as temporary CFG handles. These
  // labels are not retained as semantic authority after BIR finalization.
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

std::optional<BirFunctionLowerer::PhiBlockPlanMap> BirFunctionLowerer::collect_phi_lowering_plans()
    const {
  PhiBlockPlanMap plans;

  for (const auto& block : function_.blocks) {
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

      PhiLoweringPlan plan{
          .result_name = phi->result.str(),
          .type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(phi->type_str.str())),
          .boundary_type_ref = phi->boundary_value_type
                                   ? std::optional<c4c::codegen::lir::LirTypeRef>(
                                         phi->boundary_value_type->type)
                                   : std::nullopt,
      };
      if (const auto phi_type = lower_scalar_or_function_pointer_type(plan.type_text);
          phi_type.has_value()) {
        plan.kind = PhiLoweringPlan::Kind::ScalarValue;
        plan.type = *phi_type;
      } else {
        std::optional<AggregateTypeLayout> aggregate_layout;
        if (plan.boundary_type_ref && plan.boundary_type_ref->has_struct_name_id()) {
          const auto lookup = lir_to_bir_detail::lookup_backend_aggregate_type_ref_layout_result(
              *plan.boundary_type_ref, type_decls_, structured_layouts_);
          if (!lookup.used_structured_layout ||
              (lookup.layout.kind != AggregateTypeLayout::Kind::Struct &&
               lookup.layout.kind != AggregateTypeLayout::Kind::Array) ||
              lookup.layout.size_bytes == 0 || lookup.layout.align_bytes == 0) {
            return std::nullopt;
          }
          aggregate_layout = lookup.layout;
        } else {
          // Legacy no-id compatibility only: hand-built or inline aggregate PHI
          // LIR can lack a StructNameId-bearing boundary type ref. Metadata
          // bearing refs above must resolve structurally and fail closed.
          aggregate_layout =
              lower_byval_aggregate_layout(plan.type_text, type_decls_, &structured_layouts_);
        }
        if (!aggregate_layout.has_value()) {
          return std::nullopt;
        }
        plan.kind = PhiLoweringPlan::Kind::AggregateValue;
        plan.aggregate_align_bytes = aggregate_layout->align_bytes;
      }
      for (const auto& [value, label] : phi->incoming) {
        const c4c::codegen::lir::LirOperand incoming(value);
        if (plan.kind == PhiLoweringPlan::Kind::AggregateValue &&
            incoming.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
          return std::nullopt;
        }
        plan.incomings.push_back({label, incoming});
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
}  // namespace c4c::backend
