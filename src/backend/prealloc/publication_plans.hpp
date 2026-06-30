#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "control_flow.hpp"
#include "formal_publications.hpp"
#include "frame.hpp"
#include "names.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;
struct PreparedEdgePublication;
struct PreparedEdgePublicationSourceProducer;
struct PreparedEdgePublicationSourceProducerLookups;

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

enum class PreparedEdgeCopySourceFactsStatus {
  Available,
  MissingPreparedLookups,
  MissingPredecessorLabel,
  MissingSuccessorLabel,
  MissingDestinationValue,
  MissingPublication,
  AmbiguousPublication,
  PublicationUnavailable,
  EdgeMismatch,
  UnsupportedMove,
  MoveEdgeMismatch,
  PublicationMoveMismatch,
  MissingSourceValue,
  MissingSourceHome,
  MissingSourceProducer,
  MissingSourceMemoryAccess,
  IncompleteSourceMemoryAccess,
};

enum class PreparedCurrentBlockJoinParallelCopySourceStatus {
  Available,
  MissingNames,
  MissingValueLocations,
  MissingEdgePublicationLookups,
  MissingBlock,
  MissingSuccessorLabel,
};

[[nodiscard]] constexpr std::string_view prepared_edge_copy_source_facts_status_name(
    PreparedEdgeCopySourceFactsStatus status) {
  switch (status) {
    case PreparedEdgeCopySourceFactsStatus::Available:
      return "available";
    case PreparedEdgeCopySourceFactsStatus::MissingPreparedLookups:
      return "missing_prepared_lookups";
    case PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel:
      return "missing_predecessor_label";
    case PreparedEdgeCopySourceFactsStatus::MissingSuccessorLabel:
      return "missing_successor_label";
    case PreparedEdgeCopySourceFactsStatus::MissingDestinationValue:
      return "missing_destination_value";
    case PreparedEdgeCopySourceFactsStatus::MissingPublication:
      return "missing_publication";
    case PreparedEdgeCopySourceFactsStatus::AmbiguousPublication:
      return "ambiguous_publication";
    case PreparedEdgeCopySourceFactsStatus::PublicationUnavailable:
      return "publication_unavailable";
    case PreparedEdgeCopySourceFactsStatus::EdgeMismatch:
      return "edge_mismatch";
    case PreparedEdgeCopySourceFactsStatus::UnsupportedMove:
      return "unsupported_move";
    case PreparedEdgeCopySourceFactsStatus::MoveEdgeMismatch:
      return "move_edge_mismatch";
    case PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch:
      return "publication_move_mismatch";
    case PreparedEdgeCopySourceFactsStatus::MissingSourceValue:
      return "missing_source_value";
    case PreparedEdgeCopySourceFactsStatus::MissingSourceHome:
      return "missing_source_home";
    case PreparedEdgeCopySourceFactsStatus::MissingSourceProducer:
      return "missing_source_producer";
    case PreparedEdgeCopySourceFactsStatus::MissingSourceMemoryAccess:
      return "missing_source_memory_access";
    case PreparedEdgeCopySourceFactsStatus::IncompleteSourceMemoryAccess:
      return "incomplete_source_memory_access";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view
prepared_current_block_join_parallel_copy_source_status_name(
    PreparedCurrentBlockJoinParallelCopySourceStatus status) {
  switch (status) {
    case PreparedCurrentBlockJoinParallelCopySourceStatus::Available:
      return "available";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingNames:
      return "missing_names";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations:
      return "missing_value_locations";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingEdgePublicationLookups:
      return "missing_edge_publication_lookups";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingBlock:
      return "missing_block";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingSuccessorLabel:
      return "missing_successor_label";
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

enum class PreparedDirectGlobalReturnAuthorityStatus {
  Available,
  MissingNames,
  MissingReturnValue,
  UnsupportedReturnValue,
  MissingGlobalIdentity,
  MissingValueHome,
  HomeValueMismatch,
  UnsupportedHomeKind,
  MissingReturnMove,
  ReturnMoveMismatch,
  UnsupportedReturnDestination,
};

[[nodiscard]] constexpr std::string_view
prepared_direct_global_return_authority_status_name(
    PreparedDirectGlobalReturnAuthorityStatus status) {
  switch (status) {
    case PreparedDirectGlobalReturnAuthorityStatus::Available:
      return "available";
    case PreparedDirectGlobalReturnAuthorityStatus::MissingNames:
      return "missing_names";
    case PreparedDirectGlobalReturnAuthorityStatus::MissingReturnValue:
      return "missing_return_value";
    case PreparedDirectGlobalReturnAuthorityStatus::UnsupportedReturnValue:
      return "unsupported_return_value";
    case PreparedDirectGlobalReturnAuthorityStatus::MissingGlobalIdentity:
      return "missing_global_identity";
    case PreparedDirectGlobalReturnAuthorityStatus::MissingValueHome:
      return "missing_value_home";
    case PreparedDirectGlobalReturnAuthorityStatus::HomeValueMismatch:
      return "home_value_mismatch";
    case PreparedDirectGlobalReturnAuthorityStatus::UnsupportedHomeKind:
      return "unsupported_home_kind";
    case PreparedDirectGlobalReturnAuthorityStatus::MissingReturnMove:
      return "missing_return_move";
    case PreparedDirectGlobalReturnAuthorityStatus::ReturnMoveMismatch:
      return "return_move_mismatch";
    case PreparedDirectGlobalReturnAuthorityStatus::UnsupportedReturnDestination:
      return "unsupported_return_destination";
  }
  return "unknown";
}

struct PreparedDirectGlobalReturnAuthorityInputs {
  const PreparedNameTables* names = nullptr;
  const bir::Value* return_value = nullptr;
  const PreparedValueHome* value_home = nullptr;
  const PreparedMoveResolution* before_return_move = nullptr;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

struct PreparedDirectGlobalReturnAuthority {
  PreparedDirectGlobalReturnAuthorityStatus status =
      PreparedDirectGlobalReturnAuthorityStatus::MissingReturnValue;
  const bir::Value* return_value = nullptr;
  const PreparedValueHome* value_home = nullptr;
  const PreparedMoveResolution* before_return_move = nullptr;
  LinkNameId global_symbol_name = kInvalidLinkName;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  PreparedRegisterBank return_bank = PreparedRegisterBank::None;
};

enum class PreparedFusedPointerBranchPublicationStatus {
  Available,
  MissingNames,
  MissingBranchCondition,
  MissingTerminator,
  UnsupportedTerminator,
  UnsupportedBranchCondition,
  UnsupportedPredicate,
  UnsupportedCompareType,
  ConditionMismatch,
  TargetMismatch,
  MissingConditionHome,
  MissingLhsHome,
  MissingRhsHome,
  HomeValueMismatch,
  UnsupportedConditionHome,
  UnsupportedOperandHome,
  UnsupportedOperand,
};

[[nodiscard]] constexpr std::string_view
prepared_fused_pointer_branch_publication_status_name(
    PreparedFusedPointerBranchPublicationStatus status) {
  switch (status) {
    case PreparedFusedPointerBranchPublicationStatus::Available:
      return "available";
    case PreparedFusedPointerBranchPublicationStatus::MissingNames:
      return "missing_names";
    case PreparedFusedPointerBranchPublicationStatus::MissingBranchCondition:
      return "missing_branch_condition";
    case PreparedFusedPointerBranchPublicationStatus::MissingTerminator:
      return "missing_terminator";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedTerminator:
      return "unsupported_terminator";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedBranchCondition:
      return "unsupported_branch_condition";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedPredicate:
      return "unsupported_predicate";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedCompareType:
      return "unsupported_compare_type";
    case PreparedFusedPointerBranchPublicationStatus::ConditionMismatch:
      return "condition_mismatch";
    case PreparedFusedPointerBranchPublicationStatus::TargetMismatch:
      return "target_mismatch";
    case PreparedFusedPointerBranchPublicationStatus::MissingConditionHome:
      return "missing_condition_home";
    case PreparedFusedPointerBranchPublicationStatus::MissingLhsHome:
      return "missing_lhs_home";
    case PreparedFusedPointerBranchPublicationStatus::MissingRhsHome:
      return "missing_rhs_home";
    case PreparedFusedPointerBranchPublicationStatus::HomeValueMismatch:
      return "home_value_mismatch";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedConditionHome:
      return "unsupported_condition_home";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedOperandHome:
      return "unsupported_operand_home";
    case PreparedFusedPointerBranchPublicationStatus::UnsupportedOperand:
      return "unsupported_operand";
  }
  return "unknown";
}

enum class PreparedDependencyOperandRole {
  Lhs,
  Rhs,
};

[[nodiscard]] constexpr std::string_view prepared_dependency_operand_role_name(
    PreparedDependencyOperandRole role) {
  switch (role) {
    case PreparedDependencyOperandRole::Lhs:
      return "lhs";
    case PreparedDependencyOperandRole::Rhs:
      return "rhs";
  }
  return "unknown";
}

enum class PreparedDependencyOperandMaterializationPolicy {
  None,
  LoadFromStackSlot,
  RematerializeCastFromSource,
};

[[nodiscard]] constexpr std::string_view
prepared_dependency_operand_materialization_policy_name(
    PreparedDependencyOperandMaterializationPolicy policy) {
  switch (policy) {
    case PreparedDependencyOperandMaterializationPolicy::None:
      return "none";
    case PreparedDependencyOperandMaterializationPolicy::LoadFromStackSlot:
      return "load_from_stack_slot";
    case PreparedDependencyOperandMaterializationPolicy::RematerializeCastFromSource:
      return "rematerialize_cast_from_source";
  }
  return "unknown";
}

enum class PreparedDependencyOperandAuthorityStatus {
  Available,
  MissingNames,
  MissingPublication,
  UnsupportedPublication,
  MissingSourceProducer,
  UnsupportedSourceProducer,
  MissingDependencyOperand,
  OperandMismatch,
  MissingDependencyHome,
  HomeValueMismatch,
  MissingPolicy,
  MissingStackObject,
  StackObjectMismatch,
  MissingStackFreshness,
  MissingStackClobberSafety,
  MissingCastProducer,
  UnsupportedCastProducer,
  CastResultMismatch,
  MissingCastSourceHome,
  UnsupportedCastSourceHome,
  CastSourceMismatch,
  UnsupportedCastWidth,
};

[[nodiscard]] constexpr std::string_view
prepared_dependency_operand_authority_status_name(
    PreparedDependencyOperandAuthorityStatus status) {
  switch (status) {
    case PreparedDependencyOperandAuthorityStatus::Available:
      return "available";
    case PreparedDependencyOperandAuthorityStatus::MissingNames:
      return "missing_names";
    case PreparedDependencyOperandAuthorityStatus::MissingPublication:
      return "missing_publication";
    case PreparedDependencyOperandAuthorityStatus::UnsupportedPublication:
      return "unsupported_publication";
    case PreparedDependencyOperandAuthorityStatus::MissingSourceProducer:
      return "missing_source_producer";
    case PreparedDependencyOperandAuthorityStatus::UnsupportedSourceProducer:
      return "unsupported_source_producer";
    case PreparedDependencyOperandAuthorityStatus::MissingDependencyOperand:
      return "missing_dependency_operand";
    case PreparedDependencyOperandAuthorityStatus::OperandMismatch:
      return "operand_mismatch";
    case PreparedDependencyOperandAuthorityStatus::MissingDependencyHome:
      return "missing_dependency_home";
    case PreparedDependencyOperandAuthorityStatus::HomeValueMismatch:
      return "home_value_mismatch";
    case PreparedDependencyOperandAuthorityStatus::MissingPolicy:
      return "missing_policy";
    case PreparedDependencyOperandAuthorityStatus::MissingStackObject:
      return "missing_stack_object";
    case PreparedDependencyOperandAuthorityStatus::StackObjectMismatch:
      return "stack_object_mismatch";
    case PreparedDependencyOperandAuthorityStatus::MissingStackFreshness:
      return "missing_stack_freshness";
    case PreparedDependencyOperandAuthorityStatus::MissingStackClobberSafety:
      return "missing_stack_clobber_safety";
    case PreparedDependencyOperandAuthorityStatus::MissingCastProducer:
      return "missing_cast_producer";
    case PreparedDependencyOperandAuthorityStatus::UnsupportedCastProducer:
      return "unsupported_cast_producer";
    case PreparedDependencyOperandAuthorityStatus::CastResultMismatch:
      return "cast_result_mismatch";
    case PreparedDependencyOperandAuthorityStatus::MissingCastSourceHome:
      return "missing_cast_source_home";
    case PreparedDependencyOperandAuthorityStatus::UnsupportedCastSourceHome:
      return "unsupported_cast_source_home";
    case PreparedDependencyOperandAuthorityStatus::CastSourceMismatch:
      return "cast_source_mismatch";
    case PreparedDependencyOperandAuthorityStatus::UnsupportedCastWidth:
      return "unsupported_cast_width";
  }
  return "unknown";
}

struct PreparedFusedPointerBranchPublicationInputs {
  const PreparedNameTables* names = nullptr;
  const PreparedBranchCondition* branch_condition = nullptr;
  const bir::Terminator* terminator = nullptr;
  const PreparedValueHome* condition_home = nullptr;
  const PreparedValueHome* lhs_home = nullptr;
  const PreparedValueHome* rhs_home = nullptr;
};

struct PreparedFusedPointerBranchPublication {
  PreparedFusedPointerBranchPublicationStatus status =
      PreparedFusedPointerBranchPublicationStatus::MissingBranchCondition;
  const PreparedBranchCondition* branch_condition = nullptr;
  const bir::Terminator* terminator = nullptr;
  const PreparedValueHome* condition_home = nullptr;
  const PreparedValueHome* lhs_home = nullptr;
  const PreparedValueHome* rhs_home = nullptr;
  bir::BinaryOpcode predicate = bir::BinaryOpcode::Eq;
};

struct PreparedDependencyOperandAuthorityInputs {
  const PreparedNameTables* names = nullptr;
  const PreparedEdgePublication* publication = nullptr;
  const bir::Value* dependency_operand = nullptr;
  PreparedDependencyOperandRole operand_role = PreparedDependencyOperandRole::Rhs;
  const PreparedValueHome* dependency_home = nullptr;
  const PreparedStackObject* dependency_stack_object = nullptr;
  const PreparedEdgePublicationSourceProducer* cast_producer = nullptr;
  const PreparedValueHome* cast_source_home = nullptr;
  PreparedDependencyOperandMaterializationPolicy policy =
      PreparedDependencyOperandMaterializationPolicy::None;
  bool stack_slot_fresh_at_edge = false;
  bool stack_slot_clobber_safe_at_edge = false;
};

struct PreparedDependencyOperandAuthority {
  PreparedDependencyOperandAuthorityStatus status =
      PreparedDependencyOperandAuthorityStatus::MissingPublication;
  PreparedDependencyOperandMaterializationPolicy policy =
      PreparedDependencyOperandMaterializationPolicy::None;
  PreparedDependencyOperandRole operand_role = PreparedDependencyOperandRole::Rhs;
  const PreparedEdgePublication* publication = nullptr;
  const PreparedValueHome* dependency_home = nullptr;
  const PreparedStackObject* dependency_stack_object = nullptr;
  const PreparedEdgePublicationSourceProducer* cast_producer = nullptr;
  const PreparedValueHome* cast_source_home = nullptr;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<PreparedValueId> edge_source_value_id;
  ValueNameId edge_source_value_name = kInvalidValueName;
  PreparedValueId dependency_value_id = 0;
  ValueNameId dependency_value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind dependency_type =
      c4c::backend::bir::TypeKind::Void;
  std::optional<PreparedFrameSlotId> dependency_slot_id;
  std::optional<std::size_t> dependency_stack_offset_bytes;
  std::optional<std::size_t> dependency_stack_size_bytes;
  std::optional<std::size_t> dependency_stack_align_bytes;
  std::optional<PreparedValueId> cast_source_value_id;
  ValueNameId cast_source_value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind cast_source_type =
      c4c::backend::bir::TypeKind::Void;
};

struct PreparedDependencyOperandAuthorityRecord {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedEdgePublicationSourceProducerKind source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  std::optional<BlockLabelId> cast_producer_block_label;
  std::optional<std::size_t> cast_producer_instruction_index;
  PreparedValueHomeKind cast_source_home_kind = PreparedValueHomeKind::None;
  std::optional<std::string> cast_source_register_name;
  std::optional<std::int32_t> cast_source_immediate_i32;
  PreparedDependencyOperandAuthority authority;
};

struct PreparedDependencyOperandAuthorityRecords {
  std::vector<PreparedDependencyOperandAuthorityRecord> records;
};

enum class PreparedSelectEdgeSourceProducerPlacementKind {
  None,
  PredecessorEdgeConsumedSuppression,
};

[[nodiscard]] constexpr std::string_view
prepared_select_edge_source_producer_placement_kind_name(
    PreparedSelectEdgeSourceProducerPlacementKind kind) {
  switch (kind) {
    case PreparedSelectEdgeSourceProducerPlacementKind::None:
      return "none";
    case PreparedSelectEdgeSourceProducerPlacementKind::
        PredecessorEdgeConsumedSuppression:
      return "predecessor_edge_consumed_suppression";
  }
  return "unknown";
}

enum class PreparedSelectEdgeSourceProducerPlacementStatus {
  Available,
  MissingPublication,
  MissingMoveBundle,
  UnsupportedPlacementKind,
  UnsupportedPublication,
  MissingSourceProducer,
  UnsupportedSourceProducer,
  MissingProducerSite,
  MoveBundleProducerMismatch,
  UnsupportedMoveBundle,
  MissingMove,
  UnsupportedMove,
  MoveDestinationMismatch,
  AmbiguousPublication,
};

[[nodiscard]] constexpr std::string_view
prepared_select_edge_source_producer_placement_status_name(
    PreparedSelectEdgeSourceProducerPlacementStatus status) {
  switch (status) {
    case PreparedSelectEdgeSourceProducerPlacementStatus::Available:
      return "available";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MissingPublication:
      return "missing_publication";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MissingMoveBundle:
      return "missing_move_bundle";
    case PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedPlacementKind:
      return "unsupported_placement_kind";
    case PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedPublication:
      return "unsupported_publication";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MissingSourceProducer:
      return "missing_source_producer";
    case PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedSourceProducer:
      return "unsupported_source_producer";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MissingProducerSite:
      return "missing_producer_site";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MoveBundleProducerMismatch:
      return "move_bundle_producer_mismatch";
    case PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedMoveBundle:
      return "unsupported_move_bundle";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MissingMove:
      return "missing_move";
    case PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedMove:
      return "unsupported_move";
    case PreparedSelectEdgeSourceProducerPlacementStatus::MoveDestinationMismatch:
      return "move_destination_mismatch";
    case PreparedSelectEdgeSourceProducerPlacementStatus::AmbiguousPublication:
      return "ambiguous_publication";
  }
  return "unknown";
}

struct PreparedSelectEdgeSourceProducerPlacementInputs {
  const PreparedEdgePublication* publication = nullptr;
  const PreparedMoveBundle* move_bundle = nullptr;
  PreparedSelectEdgeSourceProducerPlacementKind placement_kind =
      PreparedSelectEdgeSourceProducerPlacementKind::None;
  std::optional<BlockLabelId> move_bundle_block_label;
};

struct PreparedSelectEdgeSourceProducerPlacement {
  PreparedSelectEdgeSourceProducerPlacementStatus status =
      PreparedSelectEdgeSourceProducerPlacementStatus::MissingPublication;
  PreparedSelectEdgeSourceProducerPlacementKind placement_kind =
      PreparedSelectEdgeSourceProducerPlacementKind::None;
  const PreparedEdgePublication* publication = nullptr;
  const PreparedMoveBundle* move_bundle = nullptr;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  std::optional<BlockLabelId> move_bundle_block_label;
  std::size_t move_bundle_instruction_index = 0;
  std::size_t move_count = 0;
};

struct PreparedSelectEdgeSourceProducerPlacementRecord {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedSelectEdgeSourceProducerPlacement placement;
};

struct PreparedSelectEdgeSourceProducerPlacementRecords {
  std::vector<PreparedSelectEdgeSourceProducerPlacementRecord> records;
};

enum class PreparedSelectCarrierAliasAuthorityStatus {
  Available,
  MissingNames,
  MissingControlFlow,
  MissingBirFunction,
  MissingPublication,
  UnsupportedPublication,
  MissingSourceProducer,
  MissingJoinTransfer,
  MissingJoinBlock,
  MissingFinalCarrier,
  MissingCarrierAliases,
  UnsupportedCarrierAlias,
  MismatchedCarrierAlias,
  NonCarrierSourceUse,
};

[[nodiscard]] constexpr std::string_view
prepared_select_carrier_alias_authority_status_name(
    PreparedSelectCarrierAliasAuthorityStatus status) {
  switch (status) {
    case PreparedSelectCarrierAliasAuthorityStatus::Available:
      return "available";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingNames:
      return "missing_names";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingControlFlow:
      return "missing_control_flow";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingBirFunction:
      return "missing_bir_function";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingPublication:
      return "missing_publication";
    case PreparedSelectCarrierAliasAuthorityStatus::UnsupportedPublication:
      return "unsupported_publication";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingSourceProducer:
      return "missing_source_producer";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingJoinTransfer:
      return "missing_join_transfer";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingJoinBlock:
      return "missing_join_block";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingFinalCarrier:
      return "missing_final_carrier";
    case PreparedSelectCarrierAliasAuthorityStatus::MissingCarrierAliases:
      return "missing_carrier_aliases";
    case PreparedSelectCarrierAliasAuthorityStatus::UnsupportedCarrierAlias:
      return "unsupported_carrier_alias";
    case PreparedSelectCarrierAliasAuthorityStatus::MismatchedCarrierAlias:
      return "mismatched_carrier_alias";
    case PreparedSelectCarrierAliasAuthorityStatus::NonCarrierSourceUse:
      return "non_carrier_source_use";
  }
  return "unknown";
}

struct PreparedSelectCarrierAliasCandidate {
  const bir::SelectInst* carrier_select = nullptr;
  BlockLabelId carrier_block_label = kInvalidBlockLabel;
  std::size_t carrier_instruction_index = 0;
};

struct PreparedSelectCarrierAliasAuthorityInputs {
  const PreparedNameTables* names = nullptr;
  const PreparedControlFlowFunction* control_flow = nullptr;
  const bir::Function* function = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
  const PreparedEdgePublication* publication = nullptr;
  std::vector<PreparedSelectCarrierAliasCandidate> carrier_aliases;
};

struct PreparedSelectCarrierAlias {
  bir::Value carrier_value;
  ValueNameId carrier_value_name = kInvalidValueName;
  std::optional<PreparedValueId> carrier_value_id;
  BlockLabelId carrier_block_label = kInvalidBlockLabel;
  std::size_t carrier_instruction_index = 0;
};

struct PreparedSelectCarrierAliasAuthority {
  PreparedSelectCarrierAliasAuthorityStatus status =
      PreparedSelectCarrierAliasAuthorityStatus::MissingPublication;
  const PreparedEdgePublication* publication = nullptr;
  const PreparedJoinTransfer* join_transfer = nullptr;
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  BlockLabelId join_block_label = kInvalidBlockLabel;
  bir::Value destination_value;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  bir::Value source_value;
  std::optional<PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  PreparedEdgePublicationSourceProducerKind source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  std::vector<PreparedSelectCarrierAlias> carrier_aliases;
  bool source_use_closure_proven = false;
};

struct PreparedSelectCarrierAliasAuthorityRecord {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedSelectCarrierAliasAuthority authority;
};

struct PreparedSelectCarrierAliasAuthorityRecords {
  std::vector<PreparedSelectCarrierAliasAuthorityRecord> records;
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

struct PreparedEdgePublicationSourceProducerLookups {
  std::unordered_map<ValueNameId, PreparedEdgePublicationSourceProducer>
      producers_by_value_name;
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
  bir::MemoryLayoutAuthorityKind source_memory_layout_authority =
      bir::MemoryLayoutAuthorityKind::Unknown;
  bir::MemoryRangeVerdict source_memory_range_verdict =
      bir::MemoryRangeVerdict::UnknownCompatible;
  bir::MemoryDynamicArrayRangeVerdict source_memory_dynamic_array_verdict =
      bir::MemoryDynamicArrayRangeVerdict::Unknown;
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

struct PreparedEdgeCopySourceFacts {
  PreparedEdgeCopySourceFactsStatus status =
      PreparedEdgeCopySourceFactsStatus::MissingPublication;
  const PreparedEdgePublication* publication = nullptr;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedValueId destination_value_id = 0;
  const PreparedMoveResolution* move = nullptr;
  bir::Value destination_value;
  bir::Value source_value;
  PreparedValueId resolved_destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  bir::Value::Kind source_value_kind = bir::Value::Kind::Immediate;
  const PreparedValueHome* source_home = nullptr;
  PreparedValueHomeKind source_home_kind = PreparedValueHomeKind::None;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueHomeKind destination_home_kind = PreparedValueHomeKind::None;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
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
  bir::MemoryLayoutAuthorityKind source_memory_layout_authority =
      bir::MemoryLayoutAuthorityKind::Unknown;
  bir::MemoryRangeVerdict source_memory_range_verdict =
      bir::MemoryRangeVerdict::UnknownCompatible;
  bir::MemoryDynamicArrayRangeVerdict source_memory_dynamic_array_verdict =
      bir::MemoryDynamicArrayRangeVerdict::Unknown;
  bool source_memory_requires_address_materialization = false;
};

struct PreparedCurrentBlockJoinParallelCopySourceFact {
  PreparedEdgeCopySourceFactsStatus status =
      PreparedEdgeCopySourceFactsStatus::MissingPublication;
  const PreparedMoveBundle* bundle = nullptr;
  const PreparedMoveResolution* move = nullptr;
  const PreparedEdgePublication* publication = nullptr;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  const PreparedValueHome* source_home = nullptr;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueHomeKind source_home_kind = PreparedValueHomeKind::None;
  PreparedValueHomeKind destination_home_kind = PreparedValueHomeKind::None;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::string> destination_register_name;
  const bir::Route5CurrentBlockJoinSourceRecord* route5_join_source = nullptr;
  bir::Route5PublicationStatus route5_join_source_status =
      bir::Route5PublicationStatus::Unavailable;
  bool route5_join_source_agrees = false;
  bool source_is_incoming_expression = false;
  bool destination_is_source_value = false;
  bool source_is_source_value = false;
  bool source_shares_destination_register = false;
  bool source_home_is_stack = false;
  bool immediate_source = false;
};

struct PreparedCurrentBlockJoinParallelCopySourceFacts {
  PreparedCurrentBlockJoinParallelCopySourceStatus status =
      PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations;
  std::vector<PreparedCurrentBlockJoinParallelCopySourceFact> facts;
  std::vector<PreparedValueId> incoming_expression_value_ids;
  std::vector<ValueNameId> incoming_expression_value_names;
  std::vector<PreparedValueId> source_value_ids;
  std::vector<ValueNameId> source_value_names;
};

struct PreparedCurrentBlockJoinParallelCopySourceQueryInputs {
  const PreparedNameTables* names = nullptr;
  const PreparedRegallocFunction* regalloc = nullptr;
  const PreparedValueLocationFunction* value_locations = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
  const PreparedEdgePublicationLookups* edge_publications = nullptr;
  const bir::Route5EdgeJoinSourceIndex* route5_edge_join_sources = nullptr;
  const bir::Block* block = nullptr;
  BlockLabelId successor_label = kInvalidBlockLabel;
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

[[nodiscard]] PreparedEdgePublicationKey prepared_edge_publication_key(
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id);

[[nodiscard]] bool
prepared_edge_publication_redundant_block_entry_parallel_copy_move(
    const PreparedEdgePublication& publication,
    const PreparedMoveResolution* move);

[[nodiscard]] bool
prepared_edge_publication_matches_parallel_copy_move_source(
    const PreparedEdgePublication& publication,
    const PreparedMoveResolution& move,
    const PreparedValueHome& source_home);

[[nodiscard]] bool prepared_edge_copy_source_facts_have_materializable_producer(
    const PreparedEdgeCopySourceFacts& facts);

[[nodiscard]] bool prepared_edge_publication_source_home_matches_source(
    const PreparedEdgePublication& publication);

[[nodiscard]] bool prepared_edge_publication_source_memory_matches_access(
    const PreparedEdgePublication& publication,
    const PreparedMemoryAccess& access);

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

[[nodiscard]] PreparedAggregateStackSourceAuthority
prepare_aggregate_stack_source_authority(
    const PreparedEdgePublication* publication);

[[nodiscard]] PreparedTypedStackSourcePublication
prepare_same_width_i32_stack_source_publication(
    const PreparedEdgePublication* publication);

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

[[nodiscard]] PreparedEdgeCopySourceFacts prepare_edge_copy_source_facts(
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

[[nodiscard]] PreparedEdgeCopySourceFacts
prepare_block_entry_parallel_copy_edge_source_facts(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    const PreparedMoveResolution& move);

[[nodiscard]] PreparedCurrentBlockJoinParallelCopySourceFacts
prepare_current_block_join_parallel_copy_source_facts(
    const PreparedCurrentBlockJoinParallelCopySourceQueryInputs& inputs);

enum class PreparedScalarPublicationStatus {
  Available,
  MissingDestinationHome,
  MissingSourceValue,
  UnsupportedHomeKind,
  MissingStackSlot,
  MissingImmediatePayload,
  MissingPointerBase,
  MissingPointerDelta,
};

[[nodiscard]] constexpr std::string_view prepared_scalar_publication_status_name(
    PreparedScalarPublicationStatus status) {
  switch (status) {
    case PreparedScalarPublicationStatus::Available:
      return "available";
    case PreparedScalarPublicationStatus::MissingDestinationHome:
      return "missing_destination_home";
    case PreparedScalarPublicationStatus::MissingSourceValue:
      return "missing_source_value";
    case PreparedScalarPublicationStatus::UnsupportedHomeKind:
      return "unsupported_home_kind";
    case PreparedScalarPublicationStatus::MissingStackSlot:
      return "missing_stack_slot";
    case PreparedScalarPublicationStatus::MissingImmediatePayload:
      return "missing_immediate_payload";
    case PreparedScalarPublicationStatus::MissingPointerBase:
      return "missing_pointer_base";
    case PreparedScalarPublicationStatus::MissingPointerDelta:
      return "missing_pointer_delta";
  }
  return "unknown";
}

enum class PreparedScalarPublicationHookKind {
  None,
  RegisterHome,
  StackSlotHome,
  RematerializableImmediate,
  PointerBasePlusOffset,
};

[[nodiscard]] constexpr std::string_view prepared_scalar_publication_hook_kind_name(
    PreparedScalarPublicationHookKind kind) {
  switch (kind) {
    case PreparedScalarPublicationHookKind::None:
      return "none";
    case PreparedScalarPublicationHookKind::RegisterHome:
      return "register_home";
    case PreparedScalarPublicationHookKind::StackSlotHome:
      return "stack_slot_home";
    case PreparedScalarPublicationHookKind::RematerializableImmediate:
      return "rematerializable_immediate";
    case PreparedScalarPublicationHookKind::PointerBasePlusOffset:
      return "pointer_base_plus_offset";
  }
  return "unknown";
}

struct PreparedScalarPublicationPlan {
  PreparedScalarPublicationStatus status =
      PreparedScalarPublicationStatus::MissingDestinationHome;
  PreparedScalarPublicationHookKind hook_kind =
      PreparedScalarPublicationHookKind::None;
  bir::Value source_value;
  PreparedValueId source_value_id = 0;
  ValueNameId source_value_name = kInvalidValueName;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueHomeKind destination_home_kind = PreparedValueHomeKind::None;
  PreparedStorageEncodingKind storage_encoding = PreparedStorageEncodingKind::None;
  const PreparedBlockEntryPublication* current_block_entry_publication = nullptr;
  bool current_block_entry_publication_available = false;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<bir::Value::F128Payload> immediate_f128;
  std::optional<ValueNameId> pointer_base_value_name;
  std::optional<LinkNameId> pointer_base_symbol_name;
  std::optional<std::int64_t> pointer_byte_delta;
};

struct PreparedScalarPublicationInputs {
  const bir::Value* source_value = nullptr;
  const PreparedValueHome* destination_home = nullptr;
  const PreparedBlockEntryPublication* current_block_entry_publication = nullptr;
};

[[nodiscard]] bool prepared_scalar_publication_available(
    const PreparedScalarPublicationPlan& plan);

[[nodiscard]] PreparedStorageEncodingKind prepared_storage_encoding_from_value_home_kind(
    PreparedValueHomeKind home_kind);

[[nodiscard]] PreparedStorageEncodingKind prepared_publication_storage_encoding_from_home(
    PreparedValueHomeKind home_kind);

[[nodiscard]] PreparedScalarPublicationPlan plan_prepared_scalar_publication(
    const PreparedScalarPublicationInputs& inputs);

enum class PreparedStoreSourcePublicationStatus {
  Available,
  MissingSourceValue,
  MissingDestinationAccess,
};

[[nodiscard]] constexpr std::string_view prepared_store_source_publication_status_name(
    PreparedStoreSourcePublicationStatus status) {
  switch (status) {
    case PreparedStoreSourcePublicationStatus::Available:
      return "available";
    case PreparedStoreSourcePublicationStatus::MissingSourceValue:
      return "missing_source_value";
    case PreparedStoreSourcePublicationStatus::MissingDestinationAccess:
      return "missing_destination_access";
  }
  return "unknown";
}

enum class PreparedStoreSourcePublicationIntent {
  None,
  StoreLocalPublication,
  StoreGlobalPublication,
  PointerStoreWriteback,
};

[[nodiscard]] constexpr std::string_view prepared_store_source_publication_intent_name(
    PreparedStoreSourcePublicationIntent intent) {
  switch (intent) {
    case PreparedStoreSourcePublicationIntent::None:
      return "none";
    case PreparedStoreSourcePublicationIntent::StoreLocalPublication:
      return "store_local_publication";
    case PreparedStoreSourcePublicationIntent::StoreGlobalPublication:
      return "store_global_publication";
    case PreparedStoreSourcePublicationIntent::PointerStoreWriteback:
      return "pointer_store_writeback";
  }
  return "unknown";
}

struct PreparedStoreSourcePublicationPlan {
  PreparedStoreSourcePublicationStatus status =
      PreparedStoreSourcePublicationStatus::MissingSourceValue;
  PreparedStoreSourcePublicationIntent intent =
      PreparedStoreSourcePublicationIntent::None;

  bir::Value source_value;
  PreparedValueId source_value_id = 0;
  ValueNameId source_value_name = kInvalidValueName;

  const PreparedMemoryAccess* destination_access = nullptr;
  PreparedAddressBaseKind destination_base_kind = PreparedAddressBaseKind::None;
  std::optional<PreparedFrameSlotId> destination_frame_slot_id;
  std::optional<ValueNameId> destination_pointer_value_name;
  std::int64_t destination_byte_offset = 0;
  std::size_t destination_size_bytes = 0;
  std::size_t destination_align_bytes = 0;
  bool destination_can_use_base_plus_offset = false;
  bool destination_is_volatile = false;

  const PreparedFrameSlot* destination_frame_slot = nullptr;
  std::optional<PreparedObjectId> destination_object_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<std::size_t> destination_stack_size_bytes;
  std::optional<std::size_t> destination_stack_align_bytes;
  const PreparedStackObject* destination_stack_object = nullptr;

  const PreparedValueHome* source_home = nullptr;
  PreparedValueHomeKind source_home_kind = PreparedValueHomeKind::None;
  PreparedStorageEncodingKind source_storage_encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_size_bytes;
  std::optional<std::size_t> source_align_bytes;
  std::optional<ValueNameId> source_pointer_base_value_name;
  std::optional<LinkNameId> source_pointer_base_symbol_name;
  std::optional<std::int64_t> source_pointer_byte_delta;

  PreparedEdgePublicationSourceProducerKind source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  const bir::LoadLocalInst* source_load_local = nullptr;
  const bir::LoadGlobalInst* source_load_global = nullptr;
  const bir::CastInst* source_cast = nullptr;
  const bir::BinaryInst* source_binary = nullptr;
  const bir::SelectInst* source_select = nullptr;

  std::optional<bir::Value> recovered_source_value;
  std::optional<std::size_t> recovered_source_instruction_index;
  bool byval_load_local_source = false;
  bool direct_global_select_chain_source = false;
  bool direct_global_select_chain_root_is_select = false;
  std::optional<std::size_t> direct_global_select_chain_root_instruction_index;

  bool pending_publication = false;
  bool stack_homes_only = false;
  bool pointer_store_writeback = false;
  bool duplicate_publication = false;

  const PreparedValueHome* pointer_base_home = nullptr;
  PreparedValueHomeKind pointer_base_home_kind = PreparedValueHomeKind::None;
  std::optional<PreparedFrameSlotId> pointer_base_slot_id;
  std::optional<std::size_t> pointer_base_stack_offset_bytes;
};

struct PreparedRecoveredStoreSourcePublication {
  bir::Value stored_value;
  std::size_t instruction_index = 0;
};

struct PreparedScalarLoadLocalSourceProducer {
  const PreparedEdgePublicationSourceProducer* producer = nullptr;
  const PreparedMemoryAccess* source_access = nullptr;
};

struct PreparedStoreSourcePublicationInputs {
  const bir::Value* source_value = nullptr;
  const PreparedMemoryAccess* destination_access = nullptr;
  const PreparedValueHome* source_home = nullptr;
  const PreparedFrameSlot* destination_frame_slot = nullptr;
  const PreparedStackObject* destination_stack_object = nullptr;
  const bir::Value* recovered_source_value = nullptr;
  std::optional<std::size_t> recovered_source_instruction_index;
  bool byval_load_local_source = false;
  bool direct_global_select_chain_source = false;
  bool direct_global_select_chain_root_is_select = false;
  std::optional<std::size_t> direct_global_select_chain_root_instruction_index;
  const PreparedValueHome* pointer_base_home = nullptr;
  PreparedStoreSourcePublicationIntent intent =
      PreparedStoreSourcePublicationIntent::None;
  bool pending_publication = false;
  bool stack_homes_only = false;
  bool pointer_store_writeback = false;
  bool duplicate_publication = false;
  const PreparedEdgePublicationSourceProducer* source_producer = nullptr;
  std::optional<std::size_t> publication_instruction_index;
};

struct PreparedFixedFormalStoreSourcePublication {
  PreparedStoreSourcePublicationPlan store_source;
  PreparedFormalPublicationPlan formal_publication;
  bool fixed_formal_source = false;
};

struct PreparedStoreGlobalPublicationCandidate {
  std::size_t instruction_index = 0;
  PreparedStoreSourcePublicationPlan store_source;
};

struct PreparedStoreSourcePublicationRecord {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  PreparedStoreSourcePublicationPlan plan;
};

struct PreparedStoreSourcePublicationPlans {
  std::vector<PreparedStoreSourcePublicationRecord> records;
};

struct PreparedCallArgumentValuePublicationFact {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId call_block_label = kInvalidBlockLabel;
  std::size_t call_instruction_index = 0;
  std::size_t arg_index = 0;
  PreparedValueId argument_value_id = 0;
  ValueNameId argument_value_name = kInvalidValueName;
  PreparedFrameSlotId argument_object_slot_id = 0;
  std::size_t argument_object_stack_offset_bytes = 0;
  std::size_t argument_object_size_bytes = 0;

  BlockLabelId source_store_block_label = kInvalidBlockLabel;
  std::size_t source_store_instruction_index = 0;
  PreparedValueId payload_value_id = 0;
  ValueNameId payload_value_name = kInvalidValueName;
  bir::Value payload_value;

  PreparedFrameSlotId destination_frame_slot_id = 0;
  std::size_t destination_stack_offset_bytes = 0;
  std::size_t destination_size_bytes = 0;
};

struct PreparedCallArgumentValuePublicationPlans {
  std::vector<PreparedCallArgumentValuePublicationFact> facts;
};

[[nodiscard]] bool prepared_store_source_publication_available(
    const PreparedStoreSourcePublicationPlan& plan);

[[nodiscard]] bool prepared_store_global_publication_has_authority(
    const PreparedStoreSourcePublicationPlan& plan);

[[nodiscard]] PreparedDirectGlobalReturnAuthority
plan_prepared_direct_global_return_authority(
    const PreparedDirectGlobalReturnAuthorityInputs& inputs);

[[nodiscard]] bool prepared_direct_global_return_authority_available(
    const PreparedDirectGlobalReturnAuthority& authority);

[[nodiscard]] PreparedFusedPointerBranchPublication
plan_prepared_fused_pointer_branch_publication(
    const PreparedFusedPointerBranchPublicationInputs& inputs);

[[nodiscard]] bool prepared_fused_pointer_branch_publication_available(
    const PreparedFusedPointerBranchPublication& publication);

[[nodiscard]] PreparedDependencyOperandAuthority
plan_prepared_dependency_operand_authority(
    const PreparedDependencyOperandAuthorityInputs& inputs);

[[nodiscard]] bool prepared_dependency_operand_authority_available(
    const PreparedDependencyOperandAuthority& authority);

[[nodiscard]] PreparedDependencyOperandAuthorityRecords
collect_prepared_dependency_operand_authorities(const PreparedBirModule& prepared);

[[nodiscard]] PreparedSelectEdgeSourceProducerPlacement
plan_prepared_select_edge_source_producer_placement(
    const PreparedSelectEdgeSourceProducerPlacementInputs& inputs);

[[nodiscard]] bool prepared_select_edge_source_producer_placement_available(
    const PreparedSelectEdgeSourceProducerPlacement& placement);

[[nodiscard]] bool
prepared_select_edge_source_producer_placement_matches_move_bundle(
    const PreparedSelectEdgeSourceProducerPlacementRecord& record,
    FunctionNameId function_name,
    std::optional<BlockLabelId> move_bundle_block_label,
    const PreparedMoveBundle& move_bundle);

[[nodiscard]] PreparedSelectEdgeSourceProducerPlacementRecords
collect_prepared_select_edge_source_producer_placements(
    const PreparedBirModule& prepared);

[[nodiscard]] PreparedSelectCarrierAliasAuthority
plan_prepared_select_carrier_alias_authority(
    const PreparedSelectCarrierAliasAuthorityInputs& inputs);

[[nodiscard]] bool prepared_select_carrier_alias_authority_available(
    const PreparedSelectCarrierAliasAuthority& authority);

[[nodiscard]] PreparedSelectCarrierAliasAuthorityRecords
collect_prepared_select_carrier_alias_authorities(
    const PreparedBirModule& prepared);

[[nodiscard]] PreparedStoreSourcePublicationPlan
plan_prepared_store_source_publication(
    const PreparedStoreSourcePublicationInputs& inputs);

[[nodiscard]] PreparedStoreSourcePublicationPlan
plan_prepared_store_global_publication(
    const PreparedValueLocationFunction* value_locations,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::StoreGlobalInst& store,
    std::size_t instruction_index,
    bool pending_publication,
    bool stack_homes_only);

[[nodiscard]] std::vector<PreparedStoreGlobalPublicationCandidate>
plan_pending_prepared_store_global_publications(
    const PreparedValueLocationFunction* value_locations,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Block* block,
    std::size_t instruction_index,
    const PreparedEdgePublicationSourceProducerLookups* source_producers);

void populate_store_source_publication_plans(PreparedBirModule& prepared);

[[nodiscard]] PreparedFixedFormalStoreSourcePublication
plan_prepared_fixed_formal_store_source_publication(
    const PreparedFormalPublicationInputs& formal_inputs,
    const PreparedStoreSourcePublicationInputs& store_inputs);

[[nodiscard]] std::optional<PreparedRecoveredStoreSourcePublication>
find_prepared_recovered_narrow_store_source_for_wide_local_load(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index);

[[nodiscard]] bool
prepared_store_source_load_local_is_byval_formal_pointer_source(
    const PreparedNameTables& names,
    const bir::Function* bir_function,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducer* source_producer);

[[nodiscard]] std::optional<PreparedScalarLoadLocalSourceProducer>
find_prepared_same_block_load_local_source_producer(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccessLookups* memory_accesses,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::prepare
