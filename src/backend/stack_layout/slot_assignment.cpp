#include "slot_assignment.hpp"

#include <unordered_set>
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

std::vector<c4c::codegen::lir::LirInst> prune_dead_param_alloca_insts(
    const LirFunction& function,
    const std::vector<ParamAllocaSlotPlan>& plans) {
  std::unordered_set<std::string> dead_param_allocas;
  dead_param_allocas.reserve(plans.size());
  for (const auto& plan : plans) {
    if (!plan.needs_stack_slot) {
      dead_param_allocas.insert(plan.alloca_name);
    }
  }

  std::vector<c4c::codegen::lir::LirInst> pruned;
  pruned.reserve(function.alloca_insts.size());

  for (std::size_t index = 0; index < function.alloca_insts.size(); ++index) {
    const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[index]);
    if (alloca == nullptr || dead_param_allocas.find(alloca->result) == dead_param_allocas.end()) {
      pruned.push_back(function.alloca_insts[index]);
      continue;
    }

    if (index + 1 < function.alloca_insts.size()) {
      const auto* store = std::get_if<LirStoreOp>(&function.alloca_insts[index + 1]);
      if (store != nullptr && store->ptr == alloca->result && is_param_name(store->val)) {
        ++index;
      }
    }
  }

  return pruned;
}

}  // namespace c4c::backend::stack_layout
