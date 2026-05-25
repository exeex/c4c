#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "control_flow.hpp"
#include "value_locations.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;

struct PreparedPriorPreservedValueEntry {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const PreparedCallPreservedValue* preserved = nullptr;
};

enum class PreparedPriorPreservedValueLookupStatus {
  Found,
  NotFound,
  Ambiguous,
  InvalidPreservation,
};

struct PreparedPriorPreservedValueLookupResult {
  PreparedPriorPreservedValueLookupStatus status =
      PreparedPriorPreservedValueLookupStatus::NotFound;
  const PreparedPriorPreservedValueEntry* entry = nullptr;
  const PreparedCallPreservedValue* preserved = nullptr;
};

struct PreparedCallPlanLookups {
  std::unordered_map<std::size_t, const PreparedCallPlan*> calls_by_position;
  std::vector<std::vector<PreparedPriorPreservedValueEntry>> prior_preserved_by_value;
  std::vector<std::vector<const PreparedCallPreservedValue*>>
      first_stack_preserved_by_call_index;
  std::unordered_map<std::size_t, std::vector<PreparedCallBoundaryEffectPlan>>
      block_entry_republication_effects_by_block;
};

struct PreparedAddressMaterializationLookups {
  std::unordered_map<BlockLabelId, std::vector<const PreparedAddressMaterialization*>>
      materializations_by_block;
};

struct PreparedMoveBundleLookups {
  std::unordered_map<std::size_t, const PreparedMoveBundle*> bundles_by_position;
};

struct PreparedValueHomeLookups {
  std::unordered_map<PreparedValueId, const PreparedValueHome*> homes_by_id;
  std::unordered_map<ValueNameId, PreparedValueId> value_ids;
};

struct PreparedFunctionLookups {
  PreparedCallPlanLookups call_plans;
  PreparedAddressMaterializationLookups address_materializations;
  PreparedMoveBundleLookups move_bundles;
  PreparedValueHomeLookups value_homes;
};

[[nodiscard]] std::size_t prepared_call_position_key(std::size_t block_index,
                                                     std::size_t instruction_index);

[[nodiscard]] std::size_t prepared_move_bundle_position_key(PreparedMovePhase phase,
                                                            std::size_t block_index,
                                                            std::size_t instruction_index);

[[nodiscard]] bool prepared_prior_preserved_value_entry_position_less(
    const PreparedPriorPreservedValueEntry& lhs,
    const PreparedPriorPreservedValueEntry& rhs);

[[nodiscard]] PreparedCallPlanLookups make_prepared_call_plan_lookups(
    const PreparedBirModule& prepared,
    const PreparedCallPlansFunction* call_plans,
    const PreparedControlFlowFunction& function);

[[nodiscard]] PreparedAddressMaterializationLookups
make_prepared_address_materialization_lookups(const PreparedBirModule& prepared,
                                              FunctionNameId function_name);

[[nodiscard]] PreparedMoveBundleLookups make_prepared_move_bundle_lookups(
    const PreparedValueLocationFunction* value_locations);

[[nodiscard]] PreparedValueHomeLookups make_prepared_value_home_lookups(
    const PreparedValueLocationFunction* value_locations);

[[nodiscard]] PreparedFunctionLookups make_prepared_function_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] const PreparedCallPlan* find_indexed_prepared_call_plan(
    const PreparedCallPlanLookups* lookups,
    const PreparedCallPlansFunction* call_plans,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] const std::vector<const PreparedAddressMaterialization*>*
find_indexed_prepared_address_materializations(
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label);

[[nodiscard]] std::vector<const PreparedAddressMaterialization*>
collect_prepared_address_materializations_for_block(
    const PreparedAddressingFunction& addressing,
    BlockLabelId block_label);

[[nodiscard]] const PreparedMoveBundle* find_indexed_prepared_move_bundle(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] const PreparedCallPreservedValue*
find_latest_indexed_prior_preserved_value(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id);

[[nodiscard]] const PreparedCallPreservedValue*
find_dominating_indexed_prior_preserved_value(
    const PreparedCallPlanLookups& lookups,
    const PreparedControlFlowFunction* control_flow,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id);

[[nodiscard]] PreparedPriorPreservedValueLookupResult
find_unique_indexed_prior_preserved_value_source(
    const PreparedCallPlanLookups& lookups,
    const PreparedControlFlowFunction* control_flow,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id);

[[nodiscard]] const PreparedCallPreservedValue*
find_latest_indexed_prior_stack_preserved_value_before_instruction(
    const PreparedCallPlanLookups& lookups,
    PreparedValueId value_id,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] const std::vector<const PreparedCallPreservedValue*>*
first_indexed_stack_preserved_values_for_call(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlansFunction& call_plans,
    const PreparedCallPlan& current_call_plan);

[[nodiscard]] const std::vector<PreparedCallBoundaryEffectPlan>*
indexed_block_entry_republication_effects_for_block(
    const PreparedCallPlanLookups& lookups,
    std::size_t block_index);

}  // namespace c4c::backend::prepare
