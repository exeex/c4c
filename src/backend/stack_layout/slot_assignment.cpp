#include "slot_assignment.hpp"

#include <unordered_map>
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

const LirStoreOp* following_entry_store(const LirFunction& function, std::size_t index) {
  if (index + 1 >= function.alloca_insts.size()) {
    return nullptr;
  }
  return std::get_if<LirStoreOp>(&function.alloca_insts[index + 1]);
}

}  // namespace

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const LirFunction& function,
    const StackLayoutAnalysis& analysis) {
  std::vector<EntryAllocaSlotPlan> plans;

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
    }

    plans.push_back(std::move(plan));
  }

  return plans;
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
    if (plan_it == plans_by_alloca.end() || plan_it->second.needs_stack_slot) {
      pruned.push_back(function.alloca_insts[index]);
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
        EntryAllocaSlotPlan{plan.alloca_name, plan.needs_stack_slot, !plan.needs_stack_slot});
  }
  return prune_dead_entry_alloca_insts(function, entry_plans);
}

}  // namespace c4c::backend::stack_layout
