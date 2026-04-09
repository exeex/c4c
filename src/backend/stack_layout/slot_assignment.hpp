#pragma once

#include "analysis.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
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

struct EntryAllocaRewritePatch {
  std::vector<c4c::codegen::lir::LirInst> alloca_insts;
  std::vector<std::pair<std::string, std::string>> canonical_allocas;
};

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs);

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const StackLayoutInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans);

EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const c4c::codegen::lir::LirFunction& function,
    const std::vector<EntryAllocaSlotPlan>& plans);

EntryAllocaRewritePatch prepare_entry_alloca_rewrite_patch(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

void apply_entry_alloca_rewrite_patch(
    c4c::codegen::lir::LirFunction& function,
    const EntryAllocaRewritePatch& patch);

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis);

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
