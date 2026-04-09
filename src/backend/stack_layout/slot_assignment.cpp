#include "slot_assignment.hpp"

#include "alloca_coalescing.hpp"
#include "../../codegen/lir/call_args_ops.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirStoreOp;
using c4c::codegen::lir::LirTerminator;

bool is_param_alloca_name(std::string_view value_name) {
  return value_name.rfind("%lv.param.", 0) == 0;
}

bool is_param_name(std::string_view value_name) {
  return value_name.rfind("%p.", 0) == 0;
}

bool is_zero_initializer_store(const LirStoreOp* store) {
  return store != nullptr && store->val == "zeroinitializer";
}

bool is_zero_initializer_store(const EntryAllocaInput& alloca) {
  return alloca.paired_store_value.has_value() && *alloca.paired_store_value == "zeroinitializer";
}

bool is_scalar_alloca_type(std::string_view type_str) {
  return !type_str.empty() && type_str.front() != '[' && type_str.front() != '{' &&
         type_str.front() != '%';
}

std::vector<LirInst> build_pruned_entry_alloca_insts(
    const StackLayoutInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  std::unordered_map<std::string, EntryAllocaSlotPlan> plans_by_alloca;
  plans_by_alloca.reserve(plans.size());
  for (const auto& plan : plans) {
    plans_by_alloca.emplace(plan.alloca_name, plan);
  }

  std::vector<LirInst> pruned;
  pruned.reserve(input.entry_allocas.size() * 2);

  for (const auto& alloca : input.entry_allocas) {
    const auto plan_it = plans_by_alloca.find(alloca.alloca_name);
    if (plan_it == plans_by_alloca.end()) {
      pruned.push_back(LirAllocaOp{alloca.alloca_name, alloca.type_str, "", alloca.align});
      if (alloca.paired_store_value.has_value()) {
        pruned.push_back(LirStoreOp{alloca.type_str, *alloca.paired_store_value, alloca.alloca_name});
      }
      continue;
    }

    if (plan_it->second.needs_stack_slot) {
      pruned.push_back(LirAllocaOp{alloca.alloca_name, alloca.type_str, "", alloca.align});
      if (alloca.paired_store_value.has_value() && !plan_it->second.remove_following_entry_store) {
        pruned.push_back(LirStoreOp{alloca.type_str, *alloca.paired_store_value, alloca.alloca_name});
      }
      continue;
    }
  }

  return pruned;
}

struct AssignedEntrySlot {
  std::size_t slot = 0;
  std::string type_str;
  int align = 0;
  std::unordered_set<std::size_t> occupied_blocks;
};

template <typename Map>
void rewrite_value_name(std::string& value_name, const Map& canonical_names) {
  const auto it = canonical_names.find(value_name);
  if (it != canonical_names.end()) {
    value_name = it->second;
  }
}

template <typename Map>
void rewrite_inst_alloca_refs(c4c::codegen::lir::LirInst& inst, const Map& canonical_names) {
  std::visit(
      [&](auto& op) {
        using Op = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirMemcpyOp>) {
          rewrite_value_name(op.dst, canonical_names);
          rewrite_value_name(op.src, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaStartOp>) {
          rewrite_value_name(op.ap_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaEndOp>) {
          rewrite_value_name(op.ap_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaCopyOp>) {
          rewrite_value_name(op.dst_ptr, canonical_names);
          rewrite_value_name(op.src_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirStackRestoreOp>) {
          rewrite_value_name(op.saved_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirAbsOp>) {
          rewrite_value_name(op.arg, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirIndirectBrOp>) {
          rewrite_value_name(op.addr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirExtractValueOp>) {
          rewrite_value_name(op.agg, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirInsertValueOp>) {
          rewrite_value_name(op.agg, canonical_names);
          rewrite_value_name(op.elem, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirLoadOp>) {
          rewrite_value_name(op.ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirStoreOp>) {
          rewrite_value_name(op.val, canonical_names);
          rewrite_value_name(op.ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirCastOp>) {
          rewrite_value_name(op.operand, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirGepOp>) {
          rewrite_value_name(op.ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirBinOp>) {
          rewrite_value_name(op.lhs, canonical_names);
          rewrite_value_name(op.rhs, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirCmpOp>) {
          rewrite_value_name(op.lhs, canonical_names);
          rewrite_value_name(op.rhs, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirPhiOp>) {
          for (auto& [value, _] : op.incoming) {
            rewrite_value_name(value, canonical_names);
          }
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirSelectOp>) {
          rewrite_value_name(op.cond, canonical_names);
          rewrite_value_name(op.true_val, canonical_names);
          rewrite_value_name(op.false_val, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirInsertElementOp>) {
          rewrite_value_name(op.vec, canonical_names);
          rewrite_value_name(op.elem, canonical_names);
          rewrite_value_name(op.index, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirExtractElementOp>) {
          rewrite_value_name(op.vec, canonical_names);
          rewrite_value_name(op.index, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirShuffleVectorOp>) {
          rewrite_value_name(op.vec1, canonical_names);
          rewrite_value_name(op.vec2, canonical_names);
          rewrite_value_name(op.mask, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaArgOp>) {
          rewrite_value_name(op.ap_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirCallOp>) {
          c4c::codegen::lir::rewrite_lir_call_operands(
              op, [&](std::string_view operand) -> std::optional<std::string_view> {
                const auto it = canonical_names.find(std::string(operand));
                if (it == canonical_names.end()) {
                  return std::nullopt;
                }
                return std::string_view(it->second);
              });
        }
      },
      inst);
}

template <typename Map>
void rewrite_terminator_alloca_refs(LirTerminator& terminator, const Map& canonical_names) {
  std::visit(
      [&](auto& term) {
        using Term = std::decay_t<decltype(term)>;
        if constexpr (std::is_same_v<Term, c4c::codegen::lir::LirCondBr>) {
          rewrite_value_name(term.cond_name, canonical_names);
        } else if constexpr (std::is_same_v<Term, c4c::codegen::lir::LirRet>) {
          if (term.value_str.has_value()) {
            rewrite_value_name(*term.value_str, canonical_names);
          }
        } else if constexpr (std::is_same_v<Term, c4c::codegen::lir::LirSwitch>) {
          rewrite_value_name(term.selector_name, canonical_names);
        }
      },
      terminator);
}

}  // namespace

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs) {
  StackLayoutPlanBundle bundle;
  bundle.analysis = analyze_stack_layout(input, regalloc, callee_saved_regs);
  bundle.entry_alloca_plans = plan_entry_alloca_slots(input, bundle.analysis);
  bundle.param_alloca_plans = plan_param_alloca_slots(input, bundle.analysis);
  return bundle;
}

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs) {
  const auto regalloc =
      run_regalloc_and_merge_clobbers(liveness_input, regalloc_config, asm_clobbered);
  return build_stack_layout_plan_bundle(stack_layout_input, regalloc, callee_saved_regs);
}

EntryAllocaRewritePatch prepare_entry_alloca_rewrite_patch(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs) {
  const auto bundle = build_stack_layout_plan_bundle(
      liveness_input, stack_layout_input, regalloc_config, asm_clobbered, callee_saved_regs);
  return build_entry_alloca_rewrite_patch(stack_layout_input, bundle.entry_alloca_plans);
}

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis) {
  std::vector<EntryAllocaSlotPlan> plans;
  const auto coalescable_allocas = compute_coalescable_allocas(input, analysis);
  std::vector<AssignedEntrySlot> assigned_slots;
  std::size_t next_slot = 0;

  for (const auto& alloca : input.entry_allocas) {
    EntryAllocaSlotPlan plan;
    plan.alloca_name = alloca.alloca_name;
    plan.needs_stack_slot = true;

    const bool has_paired_store = alloca.paired_store_value.has_value();

    if (is_param_alloca_name(alloca.alloca_name)) {
      if (has_paired_store && is_param_name(*alloca.paired_store_value) &&
          analysis.is_dead_param_alloca(alloca.alloca_name)) {
        plan.needs_stack_slot = false;
        plan.remove_following_entry_store = true;
      }
    } else if (!analysis.uses_value(alloca.alloca_name)) {
      plan.needs_stack_slot = false;
      plan.remove_following_entry_store = has_paired_store;
    } else if (is_scalar_alloca_type(alloca.type_str) && is_zero_initializer_store(alloca) &&
               analysis.is_entry_alloca_overwritten_before_read(alloca.alloca_name)) {
      plan.remove_following_entry_store = true;
    }

    if (plan.needs_stack_slot && !is_param_alloca_name(alloca.alloca_name)) {
      plan.coalesced_block = coalescable_allocas.find_single_block(alloca.alloca_name);
    }
    if (plan.needs_stack_slot) {
      if (plan.coalesced_block.has_value()) {
        for (auto& assigned_slot : assigned_slots) {
          if (assigned_slot.occupied_blocks.empty() ||
              assigned_slot.type_str != alloca.type_str ||
              assigned_slot.align != alloca.align ||
              assigned_slot.occupied_blocks.find(*plan.coalesced_block) !=
                  assigned_slot.occupied_blocks.end()) {
            continue;
          }
          plan.assigned_slot = assigned_slot.slot;
          assigned_slot.occupied_blocks.insert(*plan.coalesced_block);
          break;
        }
      }
      if (!plan.assigned_slot.has_value()) {
        plan.assigned_slot = next_slot++;
        AssignedEntrySlot assigned_slot{*plan.assigned_slot, alloca.type_str, alloca.align, {}};
        if (plan.coalesced_block.has_value()) {
          assigned_slot.occupied_blocks.insert(*plan.coalesced_block);
        }
        assigned_slots.push_back(std::move(assigned_slot));
      }
    }

    plans.push_back(std::move(plan));
  }

  return plans;
}

EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const StackLayoutInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  std::unordered_map<std::size_t, std::string> canonical_by_slot;
  std::unordered_map<std::string, std::string> canonical_by_alloca;
  EntryAllocaRewritePatch patch;

  for (const auto& plan : plans) {
    if (!plan.needs_stack_slot || !plan.assigned_slot.has_value()) {
      continue;
    }
    auto [it, inserted] =
        canonical_by_slot.emplace(*plan.assigned_slot, plan.alloca_name);
    canonical_by_alloca.emplace(plan.alloca_name, it->second);
    (void)inserted;
  }

  const auto pruned_alloca_insts = build_pruned_entry_alloca_insts(input, plans);
  if (canonical_by_alloca.empty()) {
    patch.alloca_insts = pruned_alloca_insts;
    return patch;
  }

  patch.alloca_insts.reserve(pruned_alloca_insts.size());
  std::unordered_set<std::string> kept_allocas;

  for (auto inst : pruned_alloca_insts) {
    rewrite_inst_alloca_refs(inst, canonical_by_alloca);

    const auto* alloca = std::get_if<LirAllocaOp>(&inst);
    if (alloca == nullptr) {
      patch.alloca_insts.push_back(std::move(inst));
      continue;
    }

    const auto canonical_it = canonical_by_alloca.find(alloca->result);
    if (canonical_it == canonical_by_alloca.end()) {
      patch.alloca_insts.push_back(std::move(inst));
      continue;
    }
    if (!kept_allocas.insert(canonical_it->second).second) {
      continue;
    }
    patch.alloca_insts.push_back(std::move(inst));
  }

  patch.canonical_allocas.reserve(canonical_by_alloca.size());
  for (const auto& [alloca_name, canonical_name] : canonical_by_alloca) {
    patch.canonical_allocas.emplace_back(alloca_name, canonical_name);
  }
  return patch;
}

void apply_entry_alloca_rewrite_patch(
    LirFunction& function,
    const EntryAllocaRewritePatch& patch) {
  function.alloca_insts = patch.alloca_insts;
  if (patch.canonical_allocas.empty()) {
    return;
  }

  std::unordered_map<std::string, std::string> canonical_by_alloca;
  canonical_by_alloca.reserve(patch.canonical_allocas.size());
  for (const auto& [alloca_name, canonical_name] : patch.canonical_allocas) {
    canonical_by_alloca.emplace(alloca_name, canonical_name);
  }

  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      rewrite_inst_alloca_refs(inst, canonical_by_alloca);
    }
    rewrite_terminator_alloca_refs(block.terminator, canonical_by_alloca);
  }
}

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis) {
  std::vector<ParamAllocaSlotPlan> plans;
  const auto entry_plans = plan_entry_alloca_slots(input, analysis);

  for (const auto& alloca : input.entry_allocas) {
    if (!is_param_alloca_name(alloca.alloca_name) || !alloca.paired_store_value.has_value() ||
        !is_param_name(*alloca.paired_store_value)) {
      continue;
    }

    bool needs_stack_slot = true;
    for (const auto& entry_plan : entry_plans) {
      if (entry_plan.alloca_name == alloca.alloca_name) {
        needs_stack_slot = entry_plan.needs_stack_slot;
        break;
      }
    }

    plans.push_back(
        ParamAllocaSlotPlan{alloca.alloca_name, *alloca.paired_store_value, needs_stack_slot});
  }

  return plans;
}

}  // namespace c4c::backend::stack_layout
