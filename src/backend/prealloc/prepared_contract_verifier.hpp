#pragma once

#include "calls.hpp"
#include "decoded_home_storage.hpp"
#include "frame.hpp"
#include "variadic.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {

enum class PreparedContractOwnerClass {
  Coherent,
  ProducerMissing,
  ProducerIncoherent,
  TargetUnsupportedButCoherent,
  PrePreparedSemanticFailure,
};

[[nodiscard]] constexpr std::string_view prepared_contract_owner_class_name(
    PreparedContractOwnerClass owner) {
  switch (owner) {
    case PreparedContractOwnerClass::Coherent:
      return "coherent";
    case PreparedContractOwnerClass::ProducerMissing:
      return "producer_missing";
    case PreparedContractOwnerClass::ProducerIncoherent:
      return "producer_incoherent";
    case PreparedContractOwnerClass::TargetUnsupportedButCoherent:
      return "target_unsupported_but_coherent";
    case PreparedContractOwnerClass::PrePreparedSemanticFailure:
      return "pre_prepared_semantic_failure";
  }
  return "unknown";
}

enum class PreparedContractFactFamily {
  ValueHomeTypedStorage,
  ValueMaterializationFact,
  CallBoundaryArgumentResultPlan,
  CallArgumentTypedRoute,
  VariadicEntryHelperOperandHomes,
  StorageObjectExtent,
  StorageAlignment,
  StorageByteRange,
  StorageAliasAuthority,
  MemoryAccessProvenance,
  ObjectLabel,
  ObjectPublicationIdentity,
  ObjectEmittedBytes,
  ObjectZeroFill,
  ObjectRelocation,
  ObjectByteRange,
  UnsupportedObjectDataMarker,
};

[[nodiscard]] constexpr std::string_view prepared_contract_fact_family_name(
    PreparedContractFactFamily family) {
  switch (family) {
    case PreparedContractFactFamily::ValueHomeTypedStorage:
      return "value_home_typed_storage";
    case PreparedContractFactFamily::ValueMaterializationFact:
      return "value_materialization_fact";
    case PreparedContractFactFamily::CallBoundaryArgumentResultPlan:
      return "call_boundary_argument_result_plan";
    case PreparedContractFactFamily::CallArgumentTypedRoute:
      return "call_argument_typed_route";
    case PreparedContractFactFamily::VariadicEntryHelperOperandHomes:
      return "variadic_entry_helper_operand_homes";
    case PreparedContractFactFamily::StorageObjectExtent:
      return "storage_object_extent";
    case PreparedContractFactFamily::StorageAlignment:
      return "storage_alignment";
    case PreparedContractFactFamily::StorageByteRange:
      return "storage_byte_range";
    case PreparedContractFactFamily::StorageAliasAuthority:
      return "storage_alias_authority";
    case PreparedContractFactFamily::MemoryAccessProvenance:
      return "memory_access_provenance";
    case PreparedContractFactFamily::ObjectLabel:
      return "object_label";
    case PreparedContractFactFamily::ObjectPublicationIdentity:
      return "object_publication_identity";
    case PreparedContractFactFamily::ObjectEmittedBytes:
      return "object_emitted_bytes";
    case PreparedContractFactFamily::ObjectZeroFill:
      return "object_zero_fill";
    case PreparedContractFactFamily::ObjectRelocation:
      return "object_relocation";
    case PreparedContractFactFamily::ObjectByteRange:
      return "object_byte_range";
    case PreparedContractFactFamily::UnsupportedObjectDataMarker:
      return "unsupported_object_data_marker";
  }
  return "unknown";
}

enum class PreparedSelectedLocalStorageContractStatus {
  Coherent,
  MissingExtent,
  MissingAlignment,
  MissingByteRange,
  MissingAliasAuthority,
  MissingMemoryProvenance,
  ConflictingExtent,
  ConflictingAlignment,
  ConflictingByteRange,
  ConflictingAliasAuthority,
  ConflictingMemoryProvenance,
  UnsupportedButCoherent,
};

enum class PreparedRematerializableIntegerImmediateContractStatus {
  Coherent,
  MissingValueHome,
  MissingValueId,
  MissingFunctionName,
  MissingValueName,
  MissingImmediatePayload,
  ConflictingHomeKind,
  ConflictingCrossFamilyPayload,
};

[[nodiscard]] constexpr std::string_view
prepared_rematerializable_integer_immediate_contract_status_name(
    PreparedRematerializableIntegerImmediateContractStatus status) {
  switch (status) {
    case PreparedRematerializableIntegerImmediateContractStatus::Coherent:
      return "coherent";
    case PreparedRematerializableIntegerImmediateContractStatus::MissingValueHome:
      return "missing_value_home";
    case PreparedRematerializableIntegerImmediateContractStatus::MissingValueId:
      return "missing_value_id";
    case PreparedRematerializableIntegerImmediateContractStatus::MissingFunctionName:
      return "missing_function_name";
    case PreparedRematerializableIntegerImmediateContractStatus::MissingValueName:
      return "missing_value_name";
    case PreparedRematerializableIntegerImmediateContractStatus::MissingImmediatePayload:
      return "missing_immediate_payload";
    case PreparedRematerializableIntegerImmediateContractStatus::ConflictingHomeKind:
      return "conflicting_home_kind";
    case PreparedRematerializableIntegerImmediateContractStatus::
        ConflictingCrossFamilyPayload:
      return "conflicting_cross_family_payload";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view
prepared_selected_local_storage_contract_status_name(
    PreparedSelectedLocalStorageContractStatus status) {
  switch (status) {
    case PreparedSelectedLocalStorageContractStatus::Coherent:
      return "coherent";
    case PreparedSelectedLocalStorageContractStatus::MissingExtent:
      return "missing_extent";
    case PreparedSelectedLocalStorageContractStatus::MissingAlignment:
      return "missing_alignment";
    case PreparedSelectedLocalStorageContractStatus::MissingByteRange:
      return "missing_byte_range";
    case PreparedSelectedLocalStorageContractStatus::MissingAliasAuthority:
      return "missing_alias_authority";
    case PreparedSelectedLocalStorageContractStatus::MissingMemoryProvenance:
      return "missing_memory_provenance";
    case PreparedSelectedLocalStorageContractStatus::ConflictingExtent:
      return "conflicting_extent";
    case PreparedSelectedLocalStorageContractStatus::ConflictingAlignment:
      return "conflicting_alignment";
    case PreparedSelectedLocalStorageContractStatus::ConflictingByteRange:
      return "conflicting_byte_range";
    case PreparedSelectedLocalStorageContractStatus::ConflictingAliasAuthority:
      return "conflicting_alias_authority";
    case PreparedSelectedLocalStorageContractStatus::ConflictingMemoryProvenance:
      return "conflicting_memory_provenance";
    case PreparedSelectedLocalStorageContractStatus::UnsupportedButCoherent:
      return "unsupported_but_coherent";
  }
  return "unknown";
}

enum class PreparedSelectedObjectDataContractStatus {
  Coherent,
  MissingObjectLabel,
  MissingPublicationIdentity,
  MissingEmittedBytes,
  MissingZeroFill,
  MissingRelocation,
  MissingObjectByteRange,
  MissingUnsupportedObjectDataMarker,
  ConflictingObjectLabel,
  ConflictingPublicationIdentity,
  ConflictingEmittedBytes,
  ConflictingZeroFill,
  ConflictingRelocation,
  ConflictingObjectByteRange,
  ConflictingUnsupportedObjectDataMarker,
  UnsupportedButCoherent,
  InvalidPrePreparedInitializerSemantics,
};

[[nodiscard]] constexpr std::string_view
prepared_selected_object_data_contract_status_name(
    PreparedSelectedObjectDataContractStatus status) {
  switch (status) {
    case PreparedSelectedObjectDataContractStatus::Coherent:
      return "coherent";
    case PreparedSelectedObjectDataContractStatus::MissingObjectLabel:
      return "missing_object_label";
    case PreparedSelectedObjectDataContractStatus::MissingPublicationIdentity:
      return "missing_publication_identity";
    case PreparedSelectedObjectDataContractStatus::MissingEmittedBytes:
      return "missing_emitted_bytes";
    case PreparedSelectedObjectDataContractStatus::MissingZeroFill:
      return "missing_zero_fill";
    case PreparedSelectedObjectDataContractStatus::MissingRelocation:
      return "missing_relocation";
    case PreparedSelectedObjectDataContractStatus::MissingObjectByteRange:
      return "missing_object_byte_range";
    case PreparedSelectedObjectDataContractStatus::MissingUnsupportedObjectDataMarker:
      return "missing_unsupported_object_data_marker";
    case PreparedSelectedObjectDataContractStatus::ConflictingObjectLabel:
      return "conflicting_object_label";
    case PreparedSelectedObjectDataContractStatus::ConflictingPublicationIdentity:
      return "conflicting_publication_identity";
    case PreparedSelectedObjectDataContractStatus::ConflictingEmittedBytes:
      return "conflicting_emitted_bytes";
    case PreparedSelectedObjectDataContractStatus::ConflictingZeroFill:
      return "conflicting_zero_fill";
    case PreparedSelectedObjectDataContractStatus::ConflictingRelocation:
      return "conflicting_relocation";
    case PreparedSelectedObjectDataContractStatus::ConflictingObjectByteRange:
      return "conflicting_object_byte_range";
    case PreparedSelectedObjectDataContractStatus::
        ConflictingUnsupportedObjectDataMarker:
      return "conflicting_unsupported_object_data_marker";
    case PreparedSelectedObjectDataContractStatus::UnsupportedButCoherent:
      return "unsupported_but_coherent";
    case PreparedSelectedObjectDataContractStatus::
        InvalidPrePreparedInitializerSemantics:
      return "invalid_pre_prepared_initializer_semantics";
  }
  return "unknown";
}

enum class PreparedFrameSlotAddressSourceRouteContractStatus {
  Coherent,
  MissingRoute,
  MissingSourceSlot,
  MissingStackOffset,
  MissingExtent,
  MissingAlignment,
  ConflictingSourceHomeKind,
  PartialAddressMaterialization,
  ConflictingMaterializationFrameSlot,
  ConflictingCrossRoutePayload,
};

[[nodiscard]] constexpr std::string_view
prepared_frame_slot_address_source_route_contract_status_name(
    PreparedFrameSlotAddressSourceRouteContractStatus status) {
  switch (status) {
    case PreparedFrameSlotAddressSourceRouteContractStatus::Coherent:
      return "coherent";
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingRoute:
      return "missing_route";
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingSourceSlot:
      return "missing_source_slot";
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingStackOffset:
      return "missing_stack_offset";
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingExtent:
      return "missing_extent";
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingAlignment:
      return "missing_alignment";
    case PreparedFrameSlotAddressSourceRouteContractStatus::ConflictingSourceHomeKind:
      return "conflicting_source_home_kind";
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        PartialAddressMaterialization:
      return "partial_address_materialization";
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingMaterializationFrameSlot:
      return "conflicting_materialization_frame_slot";
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingCrossRoutePayload:
      return "conflicting_cross_route_payload";
  }
  return "unknown";
}

enum class PreparedFrameSlotValueSourceRouteContractStatus {
  Coherent,
  MissingRoute,
  MissingSourceValueId,
  MissingSourceValueName,
  MissingSourceHomeKind,
  MissingSourceSlot,
  MissingStackOffset,
  MissingExtent,
  MissingAlignment,
  ConflictingSourceHomeKind,
  ConflictingAddressMaterializationPayload,
  ConflictingCrossRoutePayload,
};

[[nodiscard]] constexpr std::string_view
prepared_frame_slot_value_source_route_contract_status_name(
    PreparedFrameSlotValueSourceRouteContractStatus status) {
  switch (status) {
    case PreparedFrameSlotValueSourceRouteContractStatus::Coherent:
      return "coherent";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingRoute:
      return "missing_route";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceValueId:
      return "missing_source_value_id";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceValueName:
      return "missing_source_value_name";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceHomeKind:
      return "missing_source_home_kind";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceSlot:
      return "missing_source_slot";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingStackOffset:
      return "missing_stack_offset";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingExtent:
      return "missing_extent";
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingAlignment:
      return "missing_alignment";
    case PreparedFrameSlotValueSourceRouteContractStatus::ConflictingSourceHomeKind:
      return "conflicting_source_home_kind";
    case PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingAddressMaterializationPayload:
      return "conflicting_address_materialization_payload";
    case PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingCrossRoutePayload:
      return "conflicting_cross_route_payload";
  }
  return "unknown";
}

enum class PreparedLocalFrameAddressMaterializationSourceRouteContractStatus {
  Coherent,
  MissingRoute,
  MissingSourceValueId,
  MissingSourceValueName,
  MissingSourceHomeKind,
  MissingSourceBaseValueId,
  MissingPointerByteDelta,
  MissingSourceSlot,
  MissingStackOffset,
  MissingExtent,
  MissingAlignment,
  MissingMaterializationLocation,
  MissingMaterializationFrameSlot,
  MissingMaterializationByteOffset,
  NegativeMaterializationByteOffset,
  ConflictingSourceHomeKind,
  ConflictingMaterializationFrameSlot,
  ConflictingMaterializationByteOffset,
  ConflictingCrossRoutePayload,
};

[[nodiscard]] constexpr std::string_view
prepared_local_frame_address_materialization_source_route_contract_status_name(
    PreparedLocalFrameAddressMaterializationSourceRouteContractStatus status) {
  switch (status) {
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        Coherent:
      return "coherent";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingRoute:
      return "missing_route";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingSourceValueId:
      return "missing_source_value_id";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingSourceValueName:
      return "missing_source_value_name";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingSourceHomeKind:
      return "missing_source_home_kind";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingSourceBaseValueId:
      return "missing_source_base_value_id";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingPointerByteDelta:
      return "missing_pointer_byte_delta";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingSourceSlot:
      return "missing_source_slot";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingStackOffset:
      return "missing_stack_offset";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingExtent:
      return "missing_extent";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingAlignment:
      return "missing_alignment";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingMaterializationLocation:
      return "missing_materialization_location";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingMaterializationFrameSlot:
      return "missing_materialization_frame_slot";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        MissingMaterializationByteOffset:
      return "missing_materialization_byte_offset";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        NegativeMaterializationByteOffset:
      return "negative_materialization_byte_offset";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        ConflictingSourceHomeKind:
      return "conflicting_source_home_kind";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        ConflictingMaterializationFrameSlot:
      return "conflicting_materialization_frame_slot";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        ConflictingMaterializationByteOffset:
      return "conflicting_materialization_byte_offset";
    case PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
        ConflictingCrossRoutePayload:
      return "conflicting_cross_route_payload";
  }
  return "unknown";
}

struct PreparedSelectedLocalStorageContractFacts {
  FunctionNameId function_name = kInvalidFunctionName;
  std::optional<PreparedObjectId> object_id;
  std::optional<PreparedFrameSlotId> frame_slot_id;
  std::size_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool has_extent = false;
  bool has_alignment = false;
  bool has_byte_range = false;
  bool alias_authority_required = false;
  bool has_alias_authority = false;
  bool has_memory_provenance = false;
  bool conflicting_extent = false;
  bool conflicting_alignment = false;
  bool conflicting_byte_range = false;
  bool conflicting_alias_authority = false;
  bool conflicting_memory_provenance = false;
  bool unsupported_but_coherent = false;
};

struct PreparedSelectedObjectDataContractFacts {
  std::optional<LinkNameId> object_label;
  std::size_t object_size_bytes = 0;
  std::size_t emitted_byte_count = 0;
  std::size_t zero_fill_byte_count = 0;
  bool has_object_label = false;
  bool has_publication_identity = false;
  bool requires_emitted_bytes = false;
  bool has_emitted_bytes = false;
  bool requires_zero_fill = false;
  bool has_zero_fill = false;
  bool requires_relocation = false;
  bool has_relocation = false;
  bool has_object_byte_range = false;
  bool requires_unsupported_marker = false;
  bool has_unsupported_marker = false;
  bool conflicting_object_label = false;
  bool conflicting_publication_identity = false;
  bool conflicting_emitted_bytes = false;
  bool conflicting_zero_fill = false;
  bool conflicting_relocation = false;
  bool conflicting_object_byte_range = false;
  bool conflicting_unsupported_marker = false;
  bool unsupported_but_coherent = false;
  bool invalid_pre_prepared_initializer_semantics = false;
};

struct PreparedContractVerificationReport {
  PreparedContractFactFamily fact_family =
      PreparedContractFactFamily::ValueHomeTypedStorage;
  PreparedContractOwnerClass owner_class = PreparedContractOwnerClass::Coherent;
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  bool fail_closed = false;
  std::string detail;
};

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_decoded_home_storage_contract(
    const PreparedDecodedHomeStorage& decoded);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_call_boundary_move_contract(
    const PreparedCallBoundaryMoveClassification& classification);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_variadic_entry_plan_contract(
    const PreparedVariadicEntryPlanFunction* entry_plan,
    FunctionNameId function_name);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_variadic_entry_helper_operand_homes_contract(
    const PreparedVariadicEntryHelperOperandHomes* homes,
    FunctionNameId function_name,
    PreparedVariadicEntryHelperKind expected_helper,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] PreparedRematerializableIntegerImmediateContractStatus
classify_prepared_rematerializable_integer_immediate_contract(
    const PreparedValueHome* home);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_rematerializable_integer_immediate_contract(
    const PreparedValueHome* home);

[[nodiscard]] PreparedSelectedLocalStorageContractStatus
classify_prepared_selected_local_storage_contract(
    const PreparedSelectedLocalStorageContractFacts& facts);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_selected_local_storage_contract(
    const PreparedSelectedLocalStorageContractFacts& facts);

[[nodiscard]] PreparedSelectedObjectDataContractStatus
classify_prepared_selected_object_data_contract(
    const PreparedSelectedObjectDataContractFacts& facts);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_selected_object_data_contract(
    const PreparedSelectedObjectDataContractFacts& facts);

[[nodiscard]] PreparedFrameSlotAddressSourceRouteContractStatus
classify_prepared_frame_slot_address_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_frame_slot_address_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection);

[[nodiscard]] PreparedFrameSlotValueSourceRouteContractStatus
classify_prepared_frame_slot_value_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_frame_slot_value_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection);

[[nodiscard]] PreparedLocalFrameAddressMaterializationSourceRouteContractStatus
classify_prepared_local_frame_address_materialization_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection);

[[nodiscard]] PreparedContractVerificationReport
verify_prepared_local_frame_address_materialization_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection);

}  // namespace c4c::backend::prepare
