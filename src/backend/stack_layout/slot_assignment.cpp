#include "slot_assignment.hpp"

#include "alloca_coalescing.hpp"
#include "../../codegen/lir/call_args.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirFunction;
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

bool is_scalar_alloca_type(std::string_view type_str) {
  return !type_str.empty() && type_str.front() != '[' && type_str.front() != '{' &&
         type_str.front() != '%';
}

const LirStoreOp* following_entry_store(const LirFunction& function, std::size_t index) {
  if (index + 1 >= function.alloca_insts.size()) {
    return nullptr;
  }
  return std::get_if<LirStoreOp>(&function.alloca_insts[index + 1]);
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

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const LirFunction& function,
    const StackLayoutAnalysis& analysis) {
  std::vector<EntryAllocaSlotPlan> plans;
  const auto coalescable_allocas = compute_coalescable_allocas(function, analysis);
  std::vector<AssignedEntrySlot> assigned_slots;
  std::size_t next_slot = 0;

  for (std::size_t index = 0; index < function.alloca_insts.size(); ++index) {
    const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[index]);
    if (alloca == nullptr) {
      continue;
    }

    EntryAllocaSlotPlan plan;
    plan.alloca_name = alloca->result;
    plan.needs_stack_slot = true;

    const auto* store = following_entry_store(function, index);
    const bool has_paired_store = store != nullptr && store->ptr == alloca->result;

    if (is_param_alloca_name(alloca->result)) {
      if (has_paired_store && is_param_name(store->val) &&
          analysis.is_dead_param_alloca(alloca->result)) {
        plan.needs_stack_slot = false;
        plan.remove_following_entry_store = true;
      }
    } else if (!analysis.uses_value(alloca->result)) {
      plan.needs_stack_slot = false;
      plan.remove_following_entry_store = has_paired_store;
    } else if (is_scalar_alloca_type(alloca->type_str) &&
               is_zero_initializer_store(store) &&
               analysis.is_entry_alloca_overwritten_before_read(alloca->result)) {
      plan.remove_following_entry_store = true;
    }

    if (plan.needs_stack_slot && !is_param_alloca_name(alloca->result)) {
      plan.coalesced_block = coalescable_allocas.find_single_block(alloca->result);
    }
    if (plan.needs_stack_slot) {
      if (plan.coalesced_block.has_value()) {
        for (auto& assigned_slot : assigned_slots) {
          if (assigned_slot.occupied_blocks.empty() ||
              assigned_slot.type_str != alloca->type_str ||
              assigned_slot.align != alloca->align ||
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
        AssignedEntrySlot assigned_slot{*plan.assigned_slot, alloca->type_str, alloca->align, {}};
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

void apply_entry_alloca_slot_plan(
    LirFunction& function,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  std::unordered_map<std::size_t, std::string> canonical_by_slot;
  std::unordered_map<std::string, std::string> canonical_by_alloca;

  for (const auto& plan : plans) {
    if (!plan.needs_stack_slot || !plan.assigned_slot.has_value()) {
      continue;
    }
    auto [it, inserted] =
        canonical_by_slot.emplace(*plan.assigned_slot, plan.alloca_name);
    canonical_by_alloca.emplace(plan.alloca_name, it->second);
    (void)inserted;
  }

  function.alloca_insts = prune_dead_entry_alloca_insts(function, plans);
  if (canonical_by_alloca.empty()) {
    return;
  }

  std::vector<c4c::codegen::lir::LirInst> rewritten_alloca_insts;
  rewritten_alloca_insts.reserve(function.alloca_insts.size());
  std::unordered_set<std::string> kept_allocas;

  for (auto inst : function.alloca_insts) {
    rewrite_inst_alloca_refs(inst, canonical_by_alloca);

    const auto* alloca = std::get_if<LirAllocaOp>(&inst);
    if (alloca == nullptr) {
      rewritten_alloca_insts.push_back(std::move(inst));
      continue;
    }

    const auto canonical_it = canonical_by_alloca.find(alloca->result);
    if (canonical_it == canonical_by_alloca.end()) {
      rewritten_alloca_insts.push_back(std::move(inst));
      continue;
    }
    if (!kept_allocas.insert(canonical_it->second).second) {
      continue;
    }
    rewritten_alloca_insts.push_back(std::move(inst));
  }

  function.alloca_insts = std::move(rewritten_alloca_insts);

  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      rewrite_inst_alloca_refs(inst, canonical_by_alloca);
    }
    rewrite_terminator_alloca_refs(block.terminator, canonical_by_alloca);
  }
}

std::vector<c4c::codegen::lir::LirInst> prune_dead_entry_alloca_insts(
    const LirFunction& function,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  std::unordered_map<std::string, EntryAllocaSlotPlan> plans_by_alloca;
  plans_by_alloca.reserve(plans.size());
  for (const auto& plan : plans) {
    plans_by_alloca.emplace(plan.alloca_name, plan);
  }

  std::vector<c4c::codegen::lir::LirInst> pruned;
  pruned.reserve(function.alloca_insts.size());

  for (std::size_t index = 0; index < function.alloca_insts.size(); ++index) {
    const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[index]);
    if (alloca == nullptr) {
      pruned.push_back(function.alloca_insts[index]);
      continue;
    }

    const auto plan_it = plans_by_alloca.find(alloca->result);
    if (plan_it == plans_by_alloca.end()) {
      pruned.push_back(function.alloca_insts[index]);
      continue;
    }

    if (plan_it->second.needs_stack_slot) {
      pruned.push_back(function.alloca_insts[index]);
      if (plan_it->second.remove_following_entry_store) {
        const auto* store = following_entry_store(function, index);
        if (store != nullptr && store->ptr == alloca->result) {
          ++index;
        }
      }
      continue;
    }

    if (plan_it->second.remove_following_entry_store) {
      const auto* store = following_entry_store(function, index);
      if (store != nullptr && store->ptr == alloca->result) {
        ++index;
      }
    }
  }

  return pruned;
}

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const LirFunction& function,
    const StackLayoutAnalysis& analysis) {
  std::vector<ParamAllocaSlotPlan> plans;
  const auto entry_plans = plan_entry_alloca_slots(function, analysis);

  for (std::size_t index = 0; index + 1 < function.alloca_insts.size(); ++index) {
    const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[index]);
    const auto* store = std::get_if<LirStoreOp>(&function.alloca_insts[index + 1]);
    if (alloca == nullptr || store == nullptr) {
      continue;
    }
    if (!is_param_alloca_name(alloca->result) || store->ptr != alloca->result ||
        !is_param_name(store->val)) {
      continue;
    }

    bool needs_stack_slot = true;
    for (const auto& entry_plan : entry_plans) {
      if (entry_plan.alloca_name == alloca->result) {
        needs_stack_slot = entry_plan.needs_stack_slot;
        break;
      }
    }

    plans.push_back(ParamAllocaSlotPlan{alloca->result, store->val, needs_stack_slot});
  }

  return plans;
}

std::vector<c4c::codegen::lir::LirInst> prune_dead_param_alloca_insts(
    const LirFunction& function,
    const std::vector<ParamAllocaSlotPlan>& plans) {
  std::vector<EntryAllocaSlotPlan> entry_plans;
  entry_plans.reserve(plans.size());
  for (const auto& plan : plans) {
    entry_plans.push_back(
        EntryAllocaSlotPlan{plan.alloca_name,
                            plan.needs_stack_slot,
                            !plan.needs_stack_slot,
                            std::nullopt,
                            std::nullopt});
  }
  return prune_dead_entry_alloca_insts(function, entry_plans);
}

}  // namespace c4c::backend::stack_layout
