#pragma once

#include "analysis.hpp"

#include <string>
#include <vector>

namespace c4c::backend::stack_layout {

struct ParamAllocaSlotPlan {
  std::string alloca_name;
  std::string param_name;
  bool needs_stack_slot = true;
};

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const c4c::codegen::lir::LirFunction& function,
    const StackLayoutAnalysis& analysis);

std::vector<c4c::codegen::lir::LirInst> prune_dead_param_alloca_insts(
    const c4c::codegen::lir::LirFunction& function,
    const std::vector<ParamAllocaSlotPlan>& plans);

}  // namespace c4c::backend::stack_layout
