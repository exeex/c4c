#include "prepared_lookups.hpp"

#include "module.hpp"

#include <algorithm>

namespace c4c::backend::prepare {
namespace {

[[nodiscard]] std::optional<std::size_t> prepared_block_index_by_label(
    const PreparedControlFlowFunction& function,
    BlockLabelId label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].block_label == label) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> prepared_block_successors(
    const PreparedControlFlowFunction& function,
    const PreparedControlFlowBlock& block) {
  std::vector<std::size_t> successors;
  auto append_label = [&](BlockLabelId label) {
    if (label == kInvalidBlockLabel) {
      return;
    }
    const auto index = prepared_block_index_by_label(function, label);
    if (index.has_value() &&
        std::find(successors.begin(), successors.end(), *index) == successors.end()) {
      successors.push_back(*index);
    }
  };

  if (block.terminator_kind == bir::TerminatorKind::Branch) {
    append_label(block.branch_target_label);
  } else if (block.terminator_kind == bir::TerminatorKind::CondBranch) {
    append_label(block.true_label);
    append_label(block.false_label);
  }
  return successors;
}

[[nodiscard]] std::vector<std::vector<bool>> make_prepared_dominance_matrix(
    const PreparedControlFlowFunction& function) {
  const std::size_t count = function.blocks.size();
  std::vector<std::vector<std::size_t>> predecessors(count);
  for (std::size_t index = 0; index < count; ++index) {
    for (const auto successor : prepared_block_successors(function, function.blocks[index])) {
      if (successor < count) {
        predecessors[successor].push_back(index);
      }
    }
  }

  std::vector<std::vector<bool>> dominates(count, std::vector<bool>(count, true));
  if (count != 0) {
    std::fill(dominates.front().begin(), dominates.front().end(), false);
    dominates.front().front() = true;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t index = 1; index < count; ++index) {
      std::vector<bool> next(count, !predecessors[index].empty());
      if (predecessors[index].empty()) {
        std::fill(next.begin(), next.end(), false);
      } else {
        for (const auto predecessor : predecessors[index]) {
          for (std::size_t candidate = 0; candidate < count; ++candidate) {
            next[candidate] = next[candidate] && dominates[predecessor][candidate];
          }
        }
      }
      next[index] = true;
      if (next != dominates[index]) {
        dominates[index] = std::move(next);
        changed = true;
      }
    }
  }
  return dominates;
}

[[nodiscard]] bool prepared_block_dominates(const PreparedControlFlowFunction& function,
                                            std::size_t dominator_index,
                                            std::size_t dominated_index) {
  const auto dominates = make_prepared_dominance_matrix(function);
  return dominated_index < dominates.size() &&
         dominator_index < dominates[dominated_index].size() &&
         dominates[dominated_index][dominator_index];
}

[[nodiscard]] bool prior_stack_preserved_reaches_call(
    const std::vector<PreparedPriorPreservedValueEntry>& prior_entries,
    const std::vector<std::vector<bool>>& dominates,
    const PreparedCallPlan& call) {
  for (const auto& prior : prior_entries) {
    if (prior.preserved == nullptr ||
        prior.preserved->route != PreparedCallPreservationRoute::StackSlot) {
      continue;
    }
    if (prior.block_index == call.block_index) {
      if (prior.instruction_index < call.instruction_index) {
        return true;
      }
      continue;
    }
    if (call.block_index < dominates.size() &&
        prior.block_index < dominates[call.block_index].size() &&
        dominates[call.block_index][prior.block_index]) {
      return true;
    }
  }
  return false;
}

}  // namespace

[[nodiscard]] std::size_t prepared_call_position_key(std::size_t block_index,
                                                     std::size_t instruction_index) {
  return (block_index << 32U) ^ instruction_index;
}

[[nodiscard]] std::size_t prepared_move_bundle_position_key(PreparedMovePhase phase,
                                                            std::size_t block_index,
                                                            std::size_t instruction_index) {
  return (static_cast<std::size_t>(phase) << 56U) ^
         (block_index << 32U) ^
         instruction_index;
}

[[nodiscard]] bool prepared_prior_preserved_value_entry_position_less(
    const PreparedPriorPreservedValueEntry& lhs,
    const PreparedPriorPreservedValueEntry& rhs) {
  if (lhs.block_index != rhs.block_index) {
    return lhs.block_index < rhs.block_index;
  }
  return lhs.instruction_index < rhs.instruction_index;
}

[[nodiscard]] PreparedCallPlanLookups make_prepared_call_plan_lookups(
    const PreparedCallPlansFunction* call_plans,
    const PreparedControlFlowFunction& function) {
  PreparedCallPlanLookups lookups;
  if (call_plans == nullptr) {
    return lookups;
  }
  lookups.calls_by_position.reserve(call_plans->calls.size());
  lookups.first_stack_preserved_by_call_index.resize(call_plans->calls.size());
  const auto dominates = make_prepared_dominance_matrix(function);
  for (std::size_t call_index = 0; call_index < call_plans->calls.size(); ++call_index) {
    const auto& call = call_plans->calls[call_index];
    lookups.calls_by_position.emplace(
        prepared_call_position_key(call.block_index, call.instruction_index), &call);
    for (const auto& preserved : call.preserved_values) {
      const bool stack_preserved =
          preserved.route == PreparedCallPreservationRoute::StackSlot &&
          preserved.value_name != kInvalidValueName &&
          preserved.slot_id.has_value() &&
          preserved.stack_offset_bytes.has_value() &&
          preserved.stack_size_bytes.has_value() &&
          *preserved.stack_size_bytes != 0;
      if (stack_preserved) {
        const auto has_reaching_prior =
            preserved.value_id < lookups.prior_preserved_by_value.size() &&
            prior_stack_preserved_reaches_call(
                lookups.prior_preserved_by_value[preserved.value_id], dominates, call);
        if (!has_reaching_prior) {
          lookups.first_stack_preserved_by_call_index[call_index].push_back(&preserved);
        }
      }
      if (preserved.route != PreparedCallPreservationRoute::Unknown) {
        if (preserved.value_id >= lookups.prior_preserved_by_value.size()) {
          lookups.prior_preserved_by_value.resize(preserved.value_id + 1U);
        }
        lookups.prior_preserved_by_value[preserved.value_id].push_back(
            PreparedPriorPreservedValueEntry{
                .block_index = call.block_index,
                .instruction_index = call.instruction_index,
                .preserved = &preserved,
            });
      }
    }
  }
  for (auto& entries : lookups.prior_preserved_by_value) {
    std::sort(entries.begin(), entries.end(),
              prepared_prior_preserved_value_entry_position_less);
  }
  return lookups;
}

[[nodiscard]] PreparedAddressMaterializationLookups
make_prepared_address_materialization_lookups(const PreparedBirModule& prepared,
                                              FunctionNameId function_name) {
  PreparedAddressMaterializationLookups lookups;
  const auto* addressing = find_prepared_addressing(prepared, function_name);
  if (addressing == nullptr) {
    return lookups;
  }
  lookups.materializations_by_block.reserve(addressing->address_materializations.size());
  for (const auto& materialization : addressing->address_materializations) {
    lookups.materializations_by_block[materialization.block_label].push_back(
        &materialization);
  }
  return lookups;
}

[[nodiscard]] PreparedMoveBundleLookups make_prepared_move_bundle_lookups(
    const PreparedValueLocationFunction* value_locations) {
  PreparedMoveBundleLookups lookups;
  if (value_locations == nullptr) {
    return lookups;
  }
  lookups.bundles_by_position.reserve(value_locations->move_bundles.size());
  for (const auto& bundle : value_locations->move_bundles) {
    lookups.bundles_by_position.emplace(
        prepared_move_bundle_position_key(bundle.phase, bundle.block_index,
                                          bundle.instruction_index),
        &bundle);
  }
  return lookups;
}

[[nodiscard]] PreparedValueHomeLookups make_prepared_value_home_lookups(
    const PreparedValueLocationFunction* value_locations) {
  PreparedValueHomeLookups lookups;
  if (value_locations == nullptr) {
    return lookups;
  }
  lookups.homes_by_id.reserve(value_locations->value_homes.size());
  lookups.value_ids.reserve(value_locations->value_homes.size());
  for (const auto& home : value_locations->value_homes) {
    lookups.homes_by_id.emplace(home.value_id, &home);
    if (home.value_name != kInvalidValueName) {
      lookups.value_ids.emplace(home.value_name, home.value_id);
    }
  }
  return lookups;
}

[[nodiscard]] PreparedFunctionLookups make_prepared_function_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  const auto* call_plans = find_prepared_call_plans(prepared, function.function_name);
  const auto* value_locations =
      find_prepared_value_location_function(prepared, function.function_name);
  return PreparedFunctionLookups{
      .call_plans = make_prepared_call_plan_lookups(call_plans, function),
      .address_materializations =
          make_prepared_address_materialization_lookups(prepared, function.function_name),
      .move_bundles = make_prepared_move_bundle_lookups(value_locations),
      .value_homes = make_prepared_value_home_lookups(value_locations),
  };
}

[[nodiscard]] const PreparedCallPlan* find_indexed_prepared_call_plan(
    const PreparedCallPlanLookups* lookups,
    const PreparedCallPlansFunction* call_plans,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (lookups != nullptr) {
    const auto it =
        lookups->calls_by_position.find(prepared_call_position_key(block_index,
                                                                   instruction_index));
    if (it != lookups->calls_by_position.end()) {
      return it->second;
    }
    return nullptr;
  }
  if (call_plans == nullptr) {
    return nullptr;
  }
  for (const auto& call : call_plans->calls) {
    if (call.block_index == block_index && call.instruction_index == instruction_index) {
      return &call;
    }
  }
  return nullptr;
}

[[nodiscard]] const std::vector<const PreparedAddressMaterialization*>*
find_indexed_prepared_address_materializations(
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->materializations_by_block.find(block_label);
  if (it == lookups->materializations_by_block.end()) {
    return nullptr;
  }
  return &it->second;
}

[[nodiscard]] std::vector<const PreparedAddressMaterialization*>
collect_prepared_address_materializations_for_block(
    const PreparedAddressingFunction& addressing,
    BlockLabelId block_label) {
  std::vector<const PreparedAddressMaterialization*> materializations;
  for (const auto& materialization : addressing.address_materializations) {
    if (materialization.block_label == block_label) {
      materializations.push_back(&materialization);
    }
  }
  return materializations;
}

[[nodiscard]] const PreparedMoveBundle* find_indexed_prepared_move_bundle(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (lookups != nullptr) {
    const auto it =
        lookups->bundles_by_position.find(prepared_move_bundle_position_key(
            phase, block_index, instruction_index));
    if (it != lookups->bundles_by_position.end()) {
      return it->second;
    }
    return nullptr;
  }
  return value_locations == nullptr ? nullptr
                                    : find_prepared_move_bundle(*value_locations,
                                                                phase,
                                                                block_index,
                                                                instruction_index);
}

[[nodiscard]] const PreparedCallPreservedValue*
find_latest_indexed_prior_preserved_value(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id) {
  if (value_id >= lookups.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = lookups.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const PreparedPriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(entries.begin(),
                             entries.end(),
                             current,
                             prepared_prior_preserved_value_entry_position_less);
  if (it == entries.begin()) {
    return nullptr;
  }
  --it;
  return it->preserved;
}

[[nodiscard]] const PreparedCallPreservedValue*
find_dominating_indexed_prior_preserved_value(
    const PreparedCallPlanLookups& lookups,
    const PreparedControlFlowFunction* control_flow,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id) {
  if (value_id >= lookups.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = lookups.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const PreparedPriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(entries.begin(),
                             entries.end(),
                             current,
                             prepared_prior_preserved_value_entry_position_less);
  while (it != entries.begin()) {
    --it;
    if (it->block_index == current_call_plan.block_index) {
      if (it->instruction_index < current_call_plan.instruction_index) {
        return it->preserved;
      }
      continue;
    }
    if (control_flow != nullptr &&
        prepared_block_dominates(*control_flow, it->block_index,
                                 current_call_plan.block_index)) {
      return it->preserved;
    }
  }
  return nullptr;
}

[[nodiscard]] const std::vector<const PreparedCallPreservedValue*>*
first_indexed_stack_preserved_values_for_call(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlansFunction& call_plans,
    const PreparedCallPlan& current_call_plan) {
  if (call_plans.calls.empty()) {
    return nullptr;
  }
  const auto* begin = call_plans.calls.data();
  const auto* end = begin + call_plans.calls.size();
  const auto* current = &current_call_plan;
  if (current >= begin && current < end) {
    return &lookups.first_stack_preserved_by_call_index
                [static_cast<std::size_t>(current - begin)];
  }
  for (std::size_t call_index = 0; call_index < call_plans.calls.size(); ++call_index) {
    const auto& call = call_plans.calls[call_index];
    if (call.block_index == current_call_plan.block_index &&
        call.instruction_index == current_call_plan.instruction_index) {
      return &lookups.first_stack_preserved_by_call_index[call_index];
    }
  }
  return nullptr;
}

}  // namespace c4c::backend::prepare
