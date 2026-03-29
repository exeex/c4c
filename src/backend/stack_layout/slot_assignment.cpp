#include "slot_assignment.hpp"

#include <variant>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirStoreOp;

bool is_param_alloca_name(std::string_view value_name) {
  return value_name.rfind("%lv.param.", 0) == 0;
}

bool is_param_name(std::string_view value_name) {
  return value_name.rfind("%p.", 0) == 0;
}

}  // namespace

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const LirFunction& function,
    const StackLayoutAnalysis& analysis) {
  std::vector<ParamAllocaSlotPlan> plans;

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

    plans.push_back(ParamAllocaSlotPlan{
        alloca->result,
        store->val,
        !analysis.is_dead_param_alloca(alloca->result),
    });
  }

  return plans;
}

}  // namespace c4c::backend::stack_layout
