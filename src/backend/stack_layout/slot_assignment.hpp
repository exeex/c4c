#pragma once

#include "analysis.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::stack_layout {

struct EntryAllocaSlotPlan {
  std::string alloca_name;
  bool needs_stack_slot = true;
  bool remove_following_entry_store = false;
  std::optional<std::size_t> coalesced_block;
  std::optional<std::size_t> assigned_slot;
};

struct ParamAllocaSlotPlan {
  std::string alloca_name;
  std::string param_name;
  bool needs_stack_slot = true;
};

struct StackLayoutPlanBundle {
  StackLayoutAnalysis analysis;
  std::vector<EntryAllocaSlotPlan> entry_alloca_plans;
  std::vector<ParamAllocaSlotPlan> param_alloca_plans;
};

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs);

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis);

void apply_entry_alloca_slot_plan(
    c4c::codegen::lir::LirFunction& function,
    const std::vector<EntryAllocaSlotPlan>& plans);

std::vector<c4c::codegen::lir::LirInst> prune_dead_entry_alloca_insts(
    const c4c::codegen::lir::LirFunction& function,
    const std::vector<EntryAllocaSlotPlan>& plans);

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis);

std::vector<c4c::codegen::lir::LirInst> prune_dead_param_alloca_insts(
    const c4c::codegen::lir::LirFunction& function,
    const std::vector<ParamAllocaSlotPlan>& plans);

}  // namespace c4c::backend::stack_layout
