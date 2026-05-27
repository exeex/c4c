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
  std::unordered_map<std::size_t, const PreparedCallArgumentPlan*>
      immediate_arguments_by_position_and_abi;
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

struct PreparedMemoryAccessLookups {
  std::unordered_map<std::size_t, const PreparedMemoryAccess*> accesses_by_position;
  std::unordered_map<ValueNameId, std::vector<const PreparedMemoryAccess*>>
      accesses_by_result_value_name;
  std::unordered_map<PreparedValueId, std::vector<const PreparedMemoryAccess*>>
      accesses_by_result_value_id;
};

struct PreparedFrameAddressOffset {
  const PreparedAddressMaterialization* materialization = nullptr;
  PreparedFrameSlotId frame_slot_id = 0;
  PreparedObjectId object_id = 0;
  std::size_t stack_offset_bytes = 0;
  std::int64_t materialization_byte_offset = 0;
};

struct PreparedAfterCallResultLaneBinding {
  const PreparedMoveBundle* move_bundle = nullptr;
  const PreparedAbiBinding* abi_binding = nullptr;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::size_t lane_index = 0;
};

struct PreparedMoveBundleLookups {
  std::unordered_map<std::size_t, const PreparedMoveBundle*> bundles_by_position;
  std::unordered_map<std::size_t, const PreparedMoveResolution*>
      before_call_argument_moves_by_position_and_abi;
  std::unordered_map<std::size_t, const PreparedMoveResolution*>
      before_return_abi_moves_by_source_and_bank;
  std::vector<PreparedAfterCallResultLaneBinding> after_call_result_lane_bindings;
  std::unordered_map<std::size_t, const PreparedAfterCallResultLaneBinding*>
      after_call_result_lane_bindings_by_position_and_value;
};

struct PreparedReturnChainLookups {
  std::unordered_map<std::size_t, ValueNameId> terminal_return_values_by_chain_value;
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
  LoadGlobal,
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
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return "load_global";
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return "cast";
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return "binary";
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return "select_materialization";
  }
  return "unknown";
}

enum class PreparedEdgePublicationSourceMemoryAccessStatus {
  Unavailable,
  Available,
  MissingPreparedMemoryAccess,
  IncompletePreparedMemoryAccess,
};

[[nodiscard]] constexpr std::string_view
prepared_edge_publication_source_memory_access_status_name(
    PreparedEdgePublicationSourceMemoryAccessStatus status) {
  switch (status) {
    case PreparedEdgePublicationSourceMemoryAccessStatus::Unavailable:
      return "unavailable";
    case PreparedEdgePublicationSourceMemoryAccessStatus::Available:
      return "available";
    case PreparedEdgePublicationSourceMemoryAccessStatus::MissingPreparedMemoryAccess:
      return "missing_prepared_memory_access";
    case PreparedEdgePublicationSourceMemoryAccessStatus::IncompletePreparedMemoryAccess:
      return "incomplete_prepared_memory_access";
  }
  return "unknown";
}

enum class PreparedAggregateStackSourceAuthorityStatus {
  Unavailable,
  Available,
  UnsupportedPublication,
  IncompleteConcreteStackSource,
  UnsupportedDestinationStorage,
  UnsupportedMoveAuthority,
  IncompleteDestinationMapping,
  MissingAggregateCopyAuthority,
};

[[nodiscard]] constexpr std::string_view
prepared_aggregate_stack_source_authority_status_name(
    PreparedAggregateStackSourceAuthorityStatus status) {
  switch (status) {
    case PreparedAggregateStackSourceAuthorityStatus::Unavailable:
      return "unavailable";
    case PreparedAggregateStackSourceAuthorityStatus::Available:
      return "available";
    case PreparedAggregateStackSourceAuthorityStatus::UnsupportedPublication:
      return "unsupported_publication";
    case PreparedAggregateStackSourceAuthorityStatus::IncompleteConcreteStackSource:
      return "incomplete_concrete_stack_source";
    case PreparedAggregateStackSourceAuthorityStatus::UnsupportedDestinationStorage:
      return "unsupported_destination_storage";
    case PreparedAggregateStackSourceAuthorityStatus::UnsupportedMoveAuthority:
      return "unsupported_move_authority";
    case PreparedAggregateStackSourceAuthorityStatus::IncompleteDestinationMapping:
      return "incomplete_destination_mapping";
    case PreparedAggregateStackSourceAuthorityStatus::MissingAggregateCopyAuthority:
      return "missing_aggregate_copy_authority";
  }
  return "unknown";
}

struct PreparedAggregateStackSourceAuthority {
  PreparedAggregateStackSourceAuthorityStatus status =
      PreparedAggregateStackSourceAuthorityStatus::Unavailable;
  PreparedValueId source_value_id = 0;
  PreparedValueId destination_value_id = 0;
  ValueNameId source_value_name = kInvalidValueName;
  ValueNameId destination_value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind source_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind destination_type = c4c::backend::bir::TypeKind::Void;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_stack_size_bytes;
  std::optional<std::size_t> source_stack_align_bytes;
  std::optional<std::size_t> copy_width_bytes;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
  bool has_destination_lane_mapping = false;
  bool has_lane_widths_and_offsets = false;
  bool partial_copy_allowed = false;
  bool has_abi_layout_reference = false;
  bool has_scratch_ownership = false;
};

struct PreparedEdgePublicationSourceProducer {
  PreparedEdgePublicationSourceProducerKind kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  const bir::LoadLocalInst* load_local = nullptr;
  const bir::LoadGlobalInst* load_global = nullptr;
  const bir::CastInst* cast = nullptr;
  const bir::BinaryInst* binary = nullptr;
  const bir::SelectInst* select = nullptr;
};

struct PreparedSameBlockScalarProducer {
  PreparedEdgePublicationSourceProducer producer;
  const bir::Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  ValueNameId value_name = kInvalidValueName;
};

struct PreparedSameBlockLoadLocalStoredValueSource {
  bir::Value stored_value;
  std::size_t store_instruction_index = 0;
  const PreparedEdgePublicationSourceProducer* load_producer = nullptr;
  const PreparedMemoryAccess* load_access = nullptr;
  const PreparedMemoryAccess* store_access = nullptr;
};

struct PreparedSameBlockGlobalLoadAccess {
  const bir::LoadGlobalInst* load_global = nullptr;
  const PreparedMemoryAccess* access = nullptr;
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
  const bir::LoadGlobalInst* source_load_global = nullptr;
  const bir::CastInst* source_cast = nullptr;
  const bir::BinaryInst* source_binary = nullptr;
  const bir::SelectInst* source_select = nullptr;
  PreparedEdgePublicationSourceMemoryAccessStatus source_memory_access_status =
      PreparedEdgePublicationSourceMemoryAccessStatus::Unavailable;
  const PreparedMemoryAccess* source_memory_access = nullptr;
  PreparedAddressBaseKind source_memory_base_kind = PreparedAddressBaseKind::None;
  std::optional<PreparedFrameSlotId> source_memory_frame_slot_id;
  std::optional<LinkNameId> source_memory_symbol_name;
  std::optional<ValueNameId> source_memory_pointer_value_name;
  std::int64_t source_memory_byte_offset = 0;
  std::size_t source_memory_size_bytes = 0;
  std::size_t source_memory_align_bytes = 0;
  bir::AddressSpace source_memory_address_space = bir::AddressSpace::Default;
  bool source_memory_is_volatile = false;
  bool source_memory_can_use_base_plus_offset = false;
  bool source_memory_requires_address_materialization = false;
  PreparedAggregateStackSourceAuthority aggregate_stack_source_authority;
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

enum class PreparedTypedStackSourcePublicationStatus {
  Available,
  MissingPublication,
  UnsupportedPublication,
  UnsupportedSourceHome,
  MissingConcreteStackSource,
  MissingSameWidthI32Type,
  UnsupportedDestinationStorage,
  UnsupportedMoveAuthority,
  MissingDestinationRegisterPlacement,
  MissingDestinationGprBank,
  MissingDestinationRegisterView,
};

[[nodiscard]] constexpr std::string_view
prepared_typed_stack_source_publication_status_name(
    PreparedTypedStackSourcePublicationStatus status) {
  switch (status) {
    case PreparedTypedStackSourcePublicationStatus::Available:
      return "available";
    case PreparedTypedStackSourcePublicationStatus::MissingPublication:
      return "missing_publication";
    case PreparedTypedStackSourcePublicationStatus::UnsupportedPublication:
      return "unsupported_publication";
    case PreparedTypedStackSourcePublicationStatus::UnsupportedSourceHome:
      return "unsupported_source_home";
    case PreparedTypedStackSourcePublicationStatus::MissingConcreteStackSource:
      return "missing_concrete_stack_source";
    case PreparedTypedStackSourcePublicationStatus::MissingSameWidthI32Type:
      return "missing_same_width_i32_type";
    case PreparedTypedStackSourcePublicationStatus::UnsupportedDestinationStorage:
      return "unsupported_destination_storage";
    case PreparedTypedStackSourcePublicationStatus::UnsupportedMoveAuthority:
      return "unsupported_move_authority";
    case PreparedTypedStackSourcePublicationStatus::MissingDestinationRegisterPlacement:
      return "missing_destination_register_placement";
    case PreparedTypedStackSourcePublicationStatus::MissingDestinationGprBank:
      return "missing_destination_gpr_bank";
    case PreparedTypedStackSourcePublicationStatus::MissingDestinationRegisterView:
      return "missing_destination_register_view";
  }
  return "unknown";
}

enum class PreparedTypedStackSourceExtensionPolicy {
  None,
  SameWidthNoExtension,
};

[[nodiscard]] constexpr std::string_view
prepared_typed_stack_source_extension_policy_name(
    PreparedTypedStackSourceExtensionPolicy policy) {
  switch (policy) {
    case PreparedTypedStackSourceExtensionPolicy::None:
      return "none";
    case PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension:
      return "same_width_no_extension";
  }
  return "unknown";
}

struct PreparedTypedStackSourcePublication {
  PreparedTypedStackSourcePublicationStatus status =
      PreparedTypedStackSourcePublicationStatus::MissingPublication;
  const PreparedEdgePublication* publication = nullptr;
  const PreparedValueHome* source_home = nullptr;
  const PreparedMoveResolution* move = nullptr;
  PreparedValueId source_value_id = 0;
  PreparedValueId destination_value_id = 0;
  ValueNameId source_value_name = kInvalidValueName;
  ValueNameId destination_value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind source_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind destination_type = c4c::backend::bir::TypeKind::Void;
  PreparedTypedStackSourceExtensionPolicy extension_policy =
      PreparedTypedStackSourceExtensionPolicy::None;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_stack_size_bytes;
  std::optional<std::size_t> source_stack_align_bytes;
  PreparedRegisterBank destination_register_bank = PreparedRegisterBank::None;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
};

struct PreparedFunctionLookups {
  PreparedCallPlanLookups call_plans;
  PreparedAddressMaterializationLookups address_materializations;
  PreparedMemoryAccessLookups memory_accesses;
  PreparedMoveBundleLookups move_bundles;
  PreparedReturnChainLookups return_chains;
  PreparedValueHomeLookups value_homes;
  PreparedEdgePublicationLookups edge_publications;
  PreparedEdgePublicationSourceProducerLookups edge_publication_source_producers;
};

[[nodiscard]] std::size_t prepared_call_position_key(std::size_t block_index,
                                                     std::size_t instruction_index);

[[nodiscard]] std::size_t prepared_move_bundle_position_key(PreparedMovePhase phase,
                                                            std::size_t block_index,
                                                            std::size_t instruction_index);
[[nodiscard]] std::size_t prepared_call_argument_position_key(
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index);
[[nodiscard]] std::size_t prepared_after_call_result_lane_position_key(
    std::size_t block_index,
    std::size_t instruction_index,
    PreparedValueId value_id);
[[nodiscard]] std::size_t prepared_before_return_abi_move_source_bank_key(
    std::size_t block_index,
    PreparedValueId source_value_id,
    PreparedRegisterBank destination_bank);
[[nodiscard]] std::size_t prepared_return_chain_value_key(std::size_t block_index,
                                                          std::size_t instruction_index,
                                                          ValueNameId value_name);
[[nodiscard]] std::size_t prepared_memory_access_position_key(
    BlockLabelId block_label,
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

[[nodiscard]] PreparedMemoryAccessLookups make_prepared_memory_access_lookups(
    const PreparedAddressingFunction* addressing,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] PreparedMoveBundleLookups make_prepared_move_bundle_lookups(
    const PreparedValueLocationFunction* value_locations);

[[nodiscard]] PreparedReturnChainLookups make_prepared_return_chain_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

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

[[nodiscard]] PreparedAggregateStackSourceAuthority
prepare_aggregate_stack_source_authority(
    const PreparedEdgePublication* publication);

[[nodiscard]] PreparedTypedStackSourcePublication
prepare_same_width_i32_stack_source_publication(
    const PreparedEdgePublication* publication);

[[nodiscard]] const PreparedCallPlan* find_indexed_prepared_call_plan(
    const PreparedCallPlanLookups* lookups,
    const PreparedCallPlansFunction* call_plans,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] const PreparedCallArgumentPlan*
find_indexed_prepared_immediate_call_argument(
    const PreparedCallPlanLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index);

[[nodiscard]] const std::vector<const PreparedAddressMaterialization*>*
find_indexed_prepared_address_materializations(
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label);

[[nodiscard]] const std::vector<const PreparedMemoryAccess*>*
find_indexed_prepared_memory_accesses_by_result_value_name(
    const PreparedMemoryAccessLookups* lookups,
    ValueNameId result_value_name);

[[nodiscard]] const PreparedMemoryAccess* find_indexed_prepared_memory_access(
    const PreparedMemoryAccessLookups* lookups,
    BlockLabelId block_label,
    std::size_t instruction_index);

[[nodiscard]] const PreparedMemoryAccess*
find_unique_indexed_prepared_memory_access_by_result_value_name(
    const PreparedMemoryAccessLookups* lookups,
    ValueNameId result_value_name);

[[nodiscard]] const std::vector<const PreparedMemoryAccess*>*
find_indexed_prepared_memory_accesses_by_result_value_id(
    const PreparedMemoryAccessLookups* lookups,
    PreparedValueId result_value_id);

[[nodiscard]] const PreparedMemoryAccess*
find_unique_indexed_prepared_memory_access_by_result_value_id(
    const PreparedMemoryAccessLookups* lookups,
    PreparedValueId result_value_id);

[[nodiscard]] std::vector<const PreparedAddressMaterialization*>
collect_prepared_address_materializations_for_block(
    const PreparedAddressingFunction& addressing,
    BlockLabelId block_label);

[[nodiscard]] std::optional<PreparedFrameAddressOffset>
find_indexed_prepared_frame_address_offset_for_value(
    const PreparedStackLayout& stack_layout,
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label,
    ValueNameId value_name,
    std::optional<std::size_t> before_or_at_instruction_index = std::nullopt);

[[nodiscard]] std::optional<PreparedFrameAddressOffset>
find_indexed_prepared_frame_address_offset_for_value_id(
    const PreparedStackLayout& stack_layout,
    const PreparedAddressMaterializationLookups* lookups,
    const PreparedValueHomeLookups* value_home_lookups,
    BlockLabelId block_label,
    PreparedValueId value_id,
    std::optional<std::size_t> before_or_at_instruction_index = std::nullopt);

[[nodiscard]] const PreparedMoveBundle* find_indexed_prepared_move_bundle(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] const PreparedMoveResolution*
find_indexed_prepared_before_call_argument_move(
    const PreparedMoveBundleLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index);

[[nodiscard]] const PreparedAfterCallResultLaneBinding*
find_indexed_prepared_after_call_result_lane_binding(
    const PreparedMoveBundleLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    PreparedValueId value_id);

[[nodiscard]] const PreparedMoveResolution*
find_prepared_before_return_abi_move_by_source_and_destination_bank(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    std::size_t block_index,
    PreparedValueId source_value_id,
    PreparedRegisterBank destination_bank);

[[nodiscard]] ValueNameId find_prepared_return_chain_terminal_value(
    const PreparedReturnChainLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId value_name);

[[nodiscard]] const PreparedEdgePublicationSourceProducer*
find_indexed_prepared_edge_publication_source_producer(
    const PreparedEdgePublicationSourceProducerLookups* lookups,
    ValueNameId value_name);

[[nodiscard]] std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    ValueNameId value_name,
    bir::TypeKind value_type,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedSameBlockGlobalLoadAccess>
find_prepared_global_load_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global);

[[nodiscard]] std::optional<PreparedSameBlockGlobalLoadAccess>
find_prepared_same_block_global_load_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    const PreparedSameBlockScalarProducer& producer);

[[nodiscard]] std::optional<PreparedSameBlockLoadLocalStoredValueSource>
find_prepared_same_block_load_local_stored_value_source(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

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
