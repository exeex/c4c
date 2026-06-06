#pragma once

#include "frame.hpp"
#include "names.hpp"
#include "regalloc.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedStorageEncodingKind {
  None,
  Register,
  FrameSlot,
  Immediate,
  ComputedAddress,
  SymbolAddress,
};

enum class PreparedCallPreservationRoute {
  Unknown,
  CalleeSavedRegister,
  StackSlot,
};

[[nodiscard]] constexpr std::string_view prepared_call_preservation_route_name(
    PreparedCallPreservationRoute route) {
  switch (route) {
    case PreparedCallPreservationRoute::Unknown:
      return "unknown";
    case PreparedCallPreservationRoute::CalleeSavedRegister:
      return "callee_saved_register";
    case PreparedCallPreservationRoute::StackSlot:
      return "stack_slot";
  }
  return "unknown";
}

enum class PreparedCallArgumentSourceSelectionKind {
  None,
  PriorPreservation,
  LocalFrameAddressMaterialization,
  FrameSlotAddress,
  FrameSlotValue,
  ByvalRegisterLane,
};

[[nodiscard]] constexpr std::string_view prepared_call_argument_source_selection_kind_name(
    PreparedCallArgumentSourceSelectionKind kind) {
  switch (kind) {
    case PreparedCallArgumentSourceSelectionKind::None:
      return "none";
    case PreparedCallArgumentSourceSelectionKind::PriorPreservation:
      return "prior_preservation";
    case PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization:
      return "local_frame_address_materialization";
    case PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
      return "frame_slot_address";
    case PreparedCallArgumentSourceSelectionKind::FrameSlotValue:
      return "frame_slot_value";
    case PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
      return "byval_register_lane";
  }
  return "unknown";
}

struct PreparedCallArgumentSourceSelection {
  PreparedCallArgumentSourceSelectionKind kind =
      PreparedCallArgumentSourceSelectionKind::None;
  std::optional<PreparedValueId> source_value_id;
  std::optional<ValueNameId> source_value_name;
  std::optional<PreparedValueHomeKind> source_home_kind;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_size_bytes;
  std::optional<std::size_t> source_align_bytes;
  std::optional<PreparedValueId> source_base_value_id;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::optional<BlockLabelId> address_materialization_block_label;
  std::optional<std::size_t> address_materialization_inst_index;
  std::optional<PreparedFrameSlotId> address_materialization_frame_slot_id;
  std::optional<std::int64_t> address_materialization_byte_offset;
  std::optional<std::size_t> preserved_call_block_index;
  std::optional<std::size_t> preserved_call_instruction_index;
  PreparedCallPreservationRoute preservation_route =
      PreparedCallPreservationRoute::Unknown;
  std::optional<std::string> preserved_register_name;
  std::optional<PreparedRegisterBank> preserved_register_bank;
  std::optional<std::size_t> preserved_register_contiguous_width;
  std::vector<std::string> preserved_occupied_register_names;
  std::optional<PreparedRegisterPlacement> preserved_register_placement;
  std::optional<PreparedFrameSlotId> preserved_stack_slot_id;
  std::optional<std::size_t> preserved_stack_offset_bytes;
  std::optional<std::size_t> preserved_stack_size_bytes;
  std::optional<std::size_t> preserved_stack_align_bytes;
  std::optional<std::size_t> preserved_callee_saved_save_index;
  std::optional<PreparedSpillSlotPlacement> preserved_spill_slot_placement;
  std::optional<std::size_t> byval_lane_extent_bytes;
  std::optional<std::size_t> byval_lane_source_instruction_index;
};

enum class PreparedAggregateTransportKind {
  None,
  StackCopy,
  ByvalRegisterLanes,
  PreservedRegisterLanes,
  Mixed,
};

[[nodiscard]] constexpr std::string_view prepared_aggregate_transport_kind_name(
    PreparedAggregateTransportKind kind) {
  switch (kind) {
    case PreparedAggregateTransportKind::None:
      return "none";
    case PreparedAggregateTransportKind::StackCopy:
      return "stack_copy";
    case PreparedAggregateTransportKind::ByvalRegisterLanes:
      return "byval_register_lanes";
    case PreparedAggregateTransportKind::PreservedRegisterLanes:
      return "preserved_register_lanes";
    case PreparedAggregateTransportKind::Mixed:
      return "mixed";
  }
  return "unknown";
}

enum class PreparedAggregateTransportChunkKind {
  RequiredPayload,
  Padding,
  FallbackOnly,
};

[[nodiscard]] constexpr std::string_view prepared_aggregate_transport_chunk_kind_name(
    PreparedAggregateTransportChunkKind kind) {
  switch (kind) {
    case PreparedAggregateTransportChunkKind::RequiredPayload:
      return "required_payload";
    case PreparedAggregateTransportChunkKind::Padding:
      return "padding";
    case PreparedAggregateTransportChunkKind::FallbackOnly:
      return "fallback_only";
  }
  return "unknown";
}

enum class PreparedAggregateTransportScratchKind {
  None,
  GeneralPurpose,
  FloatingOrSimd,
  Address,
  LaneMaterialization,
};

[[nodiscard]] constexpr std::string_view prepared_aggregate_transport_scratch_kind_name(
    PreparedAggregateTransportScratchKind kind) {
  switch (kind) {
    case PreparedAggregateTransportScratchKind::None:
      return "none";
    case PreparedAggregateTransportScratchKind::GeneralPurpose:
      return "general_purpose";
    case PreparedAggregateTransportScratchKind::FloatingOrSimd:
      return "floating_or_simd";
    case PreparedAggregateTransportScratchKind::Address:
      return "address";
    case PreparedAggregateTransportScratchKind::LaneMaterialization:
      return "lane_materialization";
  }
  return "unknown";
}

struct PreparedAggregateTransportScratchRequirement {
  PreparedAggregateTransportScratchKind kind =
      PreparedAggregateTransportScratchKind::None;
  std::size_t width_bytes = 0;
  bool may_overlap_source = false;
  bool may_overlap_destination = false;
};

struct PreparedAggregateTransportChunk {
  std::size_t chunk_index = 0;
  PreparedAggregateTransportChunkKind kind =
      PreparedAggregateTransportChunkKind::RequiredPayload;
  std::size_t payload_offset_bytes = 0;
  std::size_t source_offset_bytes = 0;
  std::size_t destination_offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 1;
  std::optional<std::size_t> preferred_width_bytes;
  std::vector<std::size_t> fallback_width_bytes;
};

struct PreparedAggregateTransportLane {
  std::size_t lane_index = 0;
  std::size_t chunk_index = 0;
  std::size_t lane_payload_offset_bytes = 0;
  std::size_t source_offset_bytes = 0;
  std::size_t destination_offset_bytes = 0;
  std::size_t lane_size_bytes = 0;
  std::optional<std::string> destination_register_name;
  std::optional<PreparedRegisterBank> destination_register_bank;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
  bool whole_register = true;
};

struct PreparedAggregateTransportPlan {
  PreparedAggregateTransportKind kind = PreparedAggregateTransportKind::None;
  std::size_t payload_size_bytes = 0;
  std::size_t payload_align_bytes = 1;
  std::size_t copy_size_bytes = 0;
  std::size_t copy_align_bytes = 1;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<std::size_t> destination_stack_size_bytes;
  std::vector<PreparedAggregateTransportChunk> chunks;
  std::vector<PreparedAggregateTransportLane> lanes;
  std::vector<PreparedAggregateTransportScratchRequirement> scratch_requirements;
};

struct PreparedDirectGlobalSelectChainDependency {
  bool contains_direct_global_load = false;
  bool root_is_select = false;
  std::optional<std::size_t> root_instruction_index;
};

[[nodiscard]] constexpr bool prepared_direct_global_select_chain_dependency_available(
    const PreparedDirectGlobalSelectChainDependency& dependency) {
  return dependency.contains_direct_global_load &&
         dependency.root_instruction_index.has_value();
}

struct PreparedCallArgumentDirectGlobalSelectChainDependency {
  bool available = false;
  ValueNameId source_value_name = kInvalidValueName;
  PreparedDirectGlobalSelectChainDependency direct_global_dependency;
};

struct PreparedCallArgumentPlan {
  std::size_t instruction_index = 0;
  std::size_t arg_index = 0;
  bool allows_local_aggregate_address_publication = false;
  PreparedRegisterBank value_bank = PreparedRegisterBank::None;
  PreparedStorageEncodingKind source_encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedValueId> source_value_id;
  std::optional<PreparedValueId> source_base_value_id;
  std::optional<bir::Value> source_literal;
  std::optional<std::string> source_symbol_name;
  std::optional<LinkNameId> source_symbol_name_id;
  std::optional<std::string> source_register_name;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<PreparedRegisterBank> source_register_bank;
  std::optional<ValueNameId> source_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::optional<std::string> destination_register_name;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<PreparedRegisterBank> destination_register_bank;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<std::size_t> destination_stack_size_bytes;
  std::optional<PreparedRegisterPlacement> source_register_placement;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
  std::optional<PreparedCallArgumentSourceSelection> source_selection;
  std::optional<PreparedAggregateTransportPlan> aggregate_transport;
  PreparedCallArgumentDirectGlobalSelectChainDependency
      direct_global_select_chain_dependency;
};

[[nodiscard]] constexpr const PreparedCallArgumentDirectGlobalSelectChainDependency*
find_prepared_call_argument_direct_global_select_chain_dependency(
    const PreparedCallArgumentPlan& argument) {
  return argument.direct_global_select_chain_dependency.available &&
                 prepared_direct_global_select_chain_dependency_available(
                     argument.direct_global_select_chain_dependency
                         .direct_global_dependency)
             ? &argument.direct_global_select_chain_dependency
             : nullptr;
}

struct PreparedCallResultPlan {
  std::size_t instruction_index = 0;
  PreparedRegisterBank value_bank = PreparedRegisterBank::None;
  PreparedMoveStorageKind source_storage_kind = PreparedMoveStorageKind::None;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<PreparedValueId> destination_value_id;
  std::optional<std::string> source_register_name;
  std::size_t source_contiguous_width = 1;
  std::vector<std::string> source_occupied_register_names;
  std::optional<PreparedRegisterBank> source_register_bank;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::string> destination_register_name;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<PreparedRegisterBank> destination_register_bank;
  std::optional<PreparedFrameSlotId> destination_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<PreparedRegisterPlacement> source_register_placement;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
  std::optional<PreparedSpillSlotPlacement> destination_spill_slot_placement;
};

struct PreparedClobberedRegister {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::string register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> placement;
};

struct PreparedCallBoundaryEffectEndpoint {
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  PreparedMoveStorageKind storage_kind = PreparedMoveStorageKind::None;
  std::optional<PreparedValueId> value_id;
  ValueNameId value_name = kInvalidValueName;
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterBank> register_bank;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> stack_size_bytes;
  std::optional<std::size_t> stack_align_bytes;
  std::optional<std::size_t> callee_saved_save_index;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedSpillSlotPlacement> spill_slot_placement;
};

struct PreparedCallPreservedValue {
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  PreparedCallPreservationRoute route = PreparedCallPreservationRoute::Unknown;
  std::optional<std::size_t> callee_saved_save_index;
  std::size_t contiguous_width = 1;
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterBank> register_bank;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> stack_size_bytes;
  std::optional<std::size_t> stack_align_bytes;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedSpillSlotPlacement> spill_slot_placement;
  PreparedCallBoundaryEffectEndpoint preservation_source;
  PreparedCallBoundaryEffectEndpoint preservation_destination;
  std::string preservation_reason;
};

enum class PreparedCallWrapperKind {
  SameModule,
  DirectExternFixedArity,
  DirectExternVariadic,
  Indirect,
};

[[nodiscard]] constexpr std::string_view prepared_call_wrapper_kind_name(
    PreparedCallWrapperKind kind) {
  switch (kind) {
    case PreparedCallWrapperKind::SameModule:
      return "same_module";
    case PreparedCallWrapperKind::DirectExternFixedArity:
      return "direct_extern_fixed_arity";
    case PreparedCallWrapperKind::DirectExternVariadic:
      return "direct_extern_variadic";
    case PreparedCallWrapperKind::Indirect:
      return "indirect";
  }
  return "unknown";
}

struct PreparedIndirectCalleePlan {
  ValueNameId value_name = kInvalidValueName;
  std::optional<PreparedValueId> value_id;
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::optional<std::string> register_name;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<ValueNameId> pointer_base_value_name;
  std::optional<std::int64_t> pointer_byte_delta;
  std::optional<PreparedRegisterPlacement> register_placement;
};

struct PreparedMemoryReturnPlan {
  std::optional<std::size_t> sret_arg_index;
  SlotNameId storage_slot_name = kInvalidSlotName;
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct PreparedOutgoingStackArgumentArea {
  std::size_t size_bytes = 0;
};

struct PreparedCallPlan {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  PreparedCallWrapperKind wrapper_kind = PreparedCallWrapperKind::Indirect;
  std::size_t variadic_fpr_arg_register_count = 0;
  bool is_indirect = false;
  std::optional<std::string> direct_callee_name;
  std::optional<PreparedIndirectCalleePlan> indirect_callee;
  std::optional<PreparedMemoryReturnPlan> memory_return;
  std::optional<PreparedOutgoingStackArgumentArea> outgoing_stack_argument_area;
  std::vector<PreparedCallArgumentPlan> arguments;
  std::optional<PreparedCallResultPlan> result;
  std::vector<PreparedCallPreservedValue> preserved_values;
  std::vector<PreparedClobberedRegister> clobbered_registers;
};

struct PreparedCallPlansFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedCallPlan> calls;
};

struct PreparedCallPlans {
  std::vector<PreparedCallPlansFunction> functions;
};

enum class PreparedCallBoundaryMoveClassificationStatus {
  Available,
  UnsupportedOpKind,
  MissingAbiIndex,
  MissingCallArgumentPlan,
  MissingCallResultPlan,
  MismatchedCallResultPlan,
  MissingAbiBinding,
};

[[nodiscard]] constexpr std::string_view prepared_call_boundary_move_classification_status_name(
    PreparedCallBoundaryMoveClassificationStatus status) {
  switch (status) {
    case PreparedCallBoundaryMoveClassificationStatus::Available:
      return "available";
    case PreparedCallBoundaryMoveClassificationStatus::UnsupportedOpKind:
      return "unsupported_op_kind";
    case PreparedCallBoundaryMoveClassificationStatus::MissingAbiIndex:
      return "missing_abi_index";
    case PreparedCallBoundaryMoveClassificationStatus::MissingCallArgumentPlan:
      return "missing_call_argument_plan";
    case PreparedCallBoundaryMoveClassificationStatus::MissingCallResultPlan:
      return "missing_call_result_plan";
    case PreparedCallBoundaryMoveClassificationStatus::MismatchedCallResultPlan:
      return "mismatched_call_result_plan";
    case PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding:
      return "missing_abi_binding";
  }
  return "unknown";
}

struct PreparedCallBoundaryMoveClassification {
  PreparedCallBoundaryMoveClassificationStatus status =
      PreparedCallBoundaryMoveClassificationStatus::Available;
  const PreparedCallPlan* call_plan = nullptr;
  const PreparedMoveBundle* bundle = nullptr;
  const PreparedMoveResolution* move = nullptr;
  PreparedMovePhase phase = PreparedMovePhase::BeforeInstruction;
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::size_t> abi_index;
  const PreparedCallArgumentPlan* argument_plan = nullptr;
  const PreparedCallResultPlan* result_plan = nullptr;
  const PreparedAbiBinding* abi_binding = nullptr;
};

enum class PreparedCallBoundaryEffectKind {
  ExplicitMove,
  PreservationHomePopulation,
  PreservationRepublication,
};

[[nodiscard]] constexpr std::string_view prepared_call_boundary_effect_kind_name(
    PreparedCallBoundaryEffectKind kind) {
  switch (kind) {
    case PreparedCallBoundaryEffectKind::ExplicitMove:
      return "explicit_move";
    case PreparedCallBoundaryEffectKind::PreservationHomePopulation:
      return "preservation_home_population";
    case PreparedCallBoundaryEffectKind::PreservationRepublication:
      return "preservation_republication";
  }
  return "unknown";
}

struct PreparedCallBoundaryEffectPlan {
  PreparedCallBoundaryEffectKind effect_kind =
      PreparedCallBoundaryEffectKind::ExplicitMove;
  PreparedMovePhase phase = PreparedMovePhase::BeforeInstruction;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::size_t order_index = 0;
  PreparedCallBoundaryMoveClassificationStatus classification_status =
      PreparedCallBoundaryMoveClassificationStatus::Available;
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::size_t> abi_index;
  PreparedCallBoundaryEffectEndpoint source;
  PreparedCallBoundaryEffectEndpoint destination;
  PreparedCallPreservationRoute preservation_route =
      PreparedCallPreservationRoute::Unknown;
  std::string reason;
};

[[nodiscard]] bool prepared_call_boundary_move_classification_available(
    const PreparedCallBoundaryMoveClassification& classification);

[[nodiscard]] PreparedCallBoundaryMoveClassification
classify_prepared_call_boundary_move(const PreparedCallPlan& call_plan,
                                     const PreparedMoveBundle& bundle,
                                     const PreparedMoveResolution& move);

[[nodiscard]] std::vector<PreparedCallBoundaryEffectPlan>
plan_prepared_call_boundary_effects(const PreparedCallPlan& call_plan,
                                    const PreparedMoveBundle* before_call_bundle,
                                    const PreparedMoveBundle* after_call_bundle);

}  // namespace c4c::backend::prepare
