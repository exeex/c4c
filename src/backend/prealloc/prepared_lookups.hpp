#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "control_flow.hpp"
#include "value_locations.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
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

enum class PreparedEdgePublicationLookupStatus {
  Available,
  MissingPredecessorLabel,
  MissingSuccessorLabel,
  MissingDestinationValue,
  MissingDestinationHome,
};

enum class PreparedEdgePublicationSourceProducerKind {
  Unknown,
  Immediate,
  LoadLocal,
  Cast,
  Binary,
  SelectMaterialization,
};

[[nodiscard]] constexpr std::string_view prepared_edge_publication_source_producer_kind_name(
    PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return "unknown";
    case PreparedEdgePublicationSourceProducerKind::Immediate:
      return "immediate";
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return "load_local";
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return "cast";
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return "binary";
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return "select_materialization";
  }
  return "unknown";
}

struct PreparedEdgePublicationSourceProducer {
  PreparedEdgePublicationSourceProducerKind kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  const bir::LoadLocalInst* load_local = nullptr;
  const bir::CastInst* cast = nullptr;
  const bir::BinaryInst* binary = nullptr;
  const bir::SelectInst* select = nullptr;
};

struct PreparedEdgePublication {
  PreparedEdgePublicationLookupStatus status =
      PreparedEdgePublicationLookupStatus::MissingDestinationValue;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  bir::Value destination_value;
  bir::Value source_value;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  bir::Value::Kind source_value_kind = bir::Value::Kind::Immediate;
  PreparedEdgePublicationSourceProducerKind source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  const bir::LoadLocalInst* source_load_local = nullptr;
  const bir::CastInst* source_cast = nullptr;
  const bir::BinaryInst* source_binary = nullptr;
  const bir::SelectInst* source_select = nullptr;
  const PreparedValueHome* source_home = nullptr;
  PreparedValueHomeKind source_home_kind = PreparedValueHomeKind::None;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueHomeKind destination_home_kind = PreparedValueHomeKind::None;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  bool source_and_destination_same_value_id = false;
  bool matching_move_coalesced_by_assigned_storage = false;
  bool matching_move_redundant_by_assigned_storage = false;
  PreparedMovePhase phase = PreparedMovePhase::BlockEntry;
  PreparedJoinTransferCarrierKind carrier_kind = PreparedJoinTransferCarrierKind::None;
  std::optional<std::size_t> parallel_copy_step_index;
  PreparedParallelCopyStepKind parallel_copy_step_kind =
      PreparedParallelCopyStepKind::Move;
  bool parallel_copy_step_uses_cycle_temp_source = false;
  bool parallel_copy_bundle_has_cycle = false;
  PreparedParallelCopyExecutionSite parallel_copy_execution_site =
      PreparedParallelCopyExecutionSite::PredecessorTerminator;
  std::optional<BlockLabelId> parallel_copy_execution_block_label;
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* edge_transfer = nullptr;
  const PreparedParallelCopyBundle* parallel_copy_bundle = nullptr;
  const PreparedMoveBundle* move_bundle = nullptr;
  const PreparedMoveResolution* move = nullptr;
};

struct PreparedEdgePublicationKey {
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedValueId destination_value_id = 0;
};

[[nodiscard]] bool operator==(const PreparedEdgePublicationKey& lhs,
                              const PreparedEdgePublicationKey& rhs);

struct PreparedEdgePublicationKeyHash {
  [[nodiscard]] std::size_t operator()(const PreparedEdgePublicationKey& key) const;
};

struct PreparedEdgePublicationLookups {
  std::vector<PreparedEdgePublication> publications;
  std::unordered_map<PreparedEdgePublicationKey,
                     std::vector<const PreparedEdgePublication*>,
                     PreparedEdgePublicationKeyHash>
      publications_by_edge_destination;
};

struct PreparedEdgePublicationSourceProducerLookups {
  std::unordered_map<ValueNameId, PreparedEdgePublicationSourceProducer>
      producers_by_value_name;
};

struct PreparedFunctionLookups {
  PreparedCallPlanLookups call_plans;
  PreparedAddressMaterializationLookups address_materializations;
  PreparedMoveBundleLookups move_bundles;
  PreparedValueHomeLookups value_homes;
  PreparedEdgePublicationLookups edge_publications;
  PreparedEdgePublicationSourceProducerLookups edge_publication_source_producers;
};

[[nodiscard]] std::size_t prepared_call_position_key(std::size_t block_index,
                                                     std::size_t instruction_index);

[[nodiscard]] std::size_t prepared_move_bundle_position_key(PreparedMovePhase phase,
                                                            std::size_t block_index,
                                                            std::size_t instruction_index);

[[nodiscard]] PreparedEdgePublicationKey prepared_edge_publication_key(
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id);

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

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] PreparedEdgePublicationSourceProducerLookups
make_prepared_edge_publication_source_producer_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] PreparedFunctionLookups make_prepared_function_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] bool prepared_value_homes_share_register_name(
    const PreparedValueHome& lhs,
    const PreparedValueHome& rhs);

[[nodiscard]] bool
prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
    const PreparedMoveResolution& move,
    PreparedValueId value_id);

[[nodiscard]] bool
prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
    const PreparedMoveResolution& move,
    const PreparedValueHome& source_home,
    const PreparedValueHome& destination_home);

[[nodiscard]] bool
prepared_edge_publication_redundant_block_entry_parallel_copy_move(
    const PreparedEdgePublication& publication,
    const PreparedMoveResolution* move);

[[nodiscard]] bool
prepared_edge_publication_matches_parallel_copy_move_source(
    const PreparedEdgePublication& publication,
    const PreparedMoveResolution& move,
    const PreparedValueHome& source_home);

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

[[nodiscard]] const PreparedEdgePublicationSourceProducer*
find_indexed_prepared_edge_publication_source_producer(
    const PreparedEdgePublicationSourceProducerLookups* lookups,
    ValueNameId value_name);

[[nodiscard]] const std::vector<const PreparedEdgePublication*>*
find_indexed_prepared_edge_publications(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id);

[[nodiscard]] const PreparedEdgePublication* find_unique_indexed_prepared_edge_publication(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id);

[[nodiscard]] const PreparedEdgePublication*
find_unique_indexed_block_entry_parallel_copy_edge_publication(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    const PreparedMoveResolution& move);

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
