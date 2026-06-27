#include "prepared_contract_verifier.hpp"

#include <sstream>

namespace c4c::backend::prepare {
namespace {

[[nodiscard]] PreparedContractOwnerClass owner_for_decoded_home_storage_status(
    const PreparedDecodedHomeStorage& decoded) {
  switch (decoded.status) {
    case PreparedDecodedHomeStorageStatus::Available:
      return PreparedContractOwnerClass::Coherent;
    case PreparedDecodedHomeStorageStatus::MissingAuthority:
      return PreparedContractOwnerClass::ProducerMissing;
    case PreparedDecodedHomeStorageStatus::MissingRegisterPlacement:
    case PreparedDecodedHomeStorageStatus::MissingFrameSlot:
    case PreparedDecodedHomeStorageStatus::MissingImmediatePayload:
    case PreparedDecodedHomeStorageStatus::MissingSymbolName:
      return PreparedContractOwnerClass::ProducerIncoherent;
    case PreparedDecodedHomeStorageStatus::UnsupportedStorageEncoding:
    case PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind:
      return PreparedContractOwnerClass::TargetUnsupportedButCoherent;
  }
  return PreparedContractOwnerClass::ProducerIncoherent;
}

[[nodiscard]] PreparedContractOwnerClass owner_for_call_boundary_status(
    PreparedCallBoundaryMoveClassificationStatus status) {
  switch (status) {
    case PreparedCallBoundaryMoveClassificationStatus::Available:
      return PreparedContractOwnerClass::Coherent;
    case PreparedCallBoundaryMoveClassificationStatus::UnsupportedOpKind:
      return PreparedContractOwnerClass::TargetUnsupportedButCoherent;
    case PreparedCallBoundaryMoveClassificationStatus::MissingCallArgumentPlan:
    case PreparedCallBoundaryMoveClassificationStatus::MissingCallResultPlan:
    case PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding:
      return PreparedContractOwnerClass::ProducerMissing;
    case PreparedCallBoundaryMoveClassificationStatus::MissingAbiIndex:
    case PreparedCallBoundaryMoveClassificationStatus::MismatchedCallResultPlan:
      return PreparedContractOwnerClass::ProducerIncoherent;
  }
  return PreparedContractOwnerClass::ProducerIncoherent;
}

[[nodiscard]] PreparedContractOwnerClass owner_for_selected_local_storage_status(
    PreparedSelectedLocalStorageContractStatus status) {
  switch (status) {
    case PreparedSelectedLocalStorageContractStatus::Coherent:
      return PreparedContractOwnerClass::Coherent;
    case PreparedSelectedLocalStorageContractStatus::MissingExtent:
    case PreparedSelectedLocalStorageContractStatus::MissingAlignment:
    case PreparedSelectedLocalStorageContractStatus::MissingByteRange:
    case PreparedSelectedLocalStorageContractStatus::MissingAliasAuthority:
    case PreparedSelectedLocalStorageContractStatus::MissingMemoryProvenance:
      return PreparedContractOwnerClass::ProducerMissing;
    case PreparedSelectedLocalStorageContractStatus::ConflictingExtent:
    case PreparedSelectedLocalStorageContractStatus::ConflictingAlignment:
    case PreparedSelectedLocalStorageContractStatus::ConflictingByteRange:
    case PreparedSelectedLocalStorageContractStatus::ConflictingAliasAuthority:
    case PreparedSelectedLocalStorageContractStatus::ConflictingMemoryProvenance:
      return PreparedContractOwnerClass::ProducerIncoherent;
    case PreparedSelectedLocalStorageContractStatus::UnsupportedButCoherent:
      return PreparedContractOwnerClass::TargetUnsupportedButCoherent;
  }
  return PreparedContractOwnerClass::ProducerIncoherent;
}

[[nodiscard]] PreparedContractOwnerClass owner_for_selected_object_data_status(
    PreparedSelectedObjectDataContractStatus status) {
  switch (status) {
    case PreparedSelectedObjectDataContractStatus::Coherent:
      return PreparedContractOwnerClass::Coherent;
    case PreparedSelectedObjectDataContractStatus::MissingObjectLabel:
    case PreparedSelectedObjectDataContractStatus::MissingPublicationIdentity:
    case PreparedSelectedObjectDataContractStatus::MissingEmittedBytes:
    case PreparedSelectedObjectDataContractStatus::MissingZeroFill:
    case PreparedSelectedObjectDataContractStatus::MissingRelocation:
    case PreparedSelectedObjectDataContractStatus::MissingObjectByteRange:
    case PreparedSelectedObjectDataContractStatus::MissingUnsupportedObjectDataMarker:
      return PreparedContractOwnerClass::ProducerMissing;
    case PreparedSelectedObjectDataContractStatus::ConflictingObjectLabel:
    case PreparedSelectedObjectDataContractStatus::ConflictingPublicationIdentity:
    case PreparedSelectedObjectDataContractStatus::ConflictingEmittedBytes:
    case PreparedSelectedObjectDataContractStatus::ConflictingZeroFill:
    case PreparedSelectedObjectDataContractStatus::ConflictingRelocation:
    case PreparedSelectedObjectDataContractStatus::ConflictingObjectByteRange:
    case PreparedSelectedObjectDataContractStatus::
        ConflictingUnsupportedObjectDataMarker:
      return PreparedContractOwnerClass::ProducerIncoherent;
    case PreparedSelectedObjectDataContractStatus::UnsupportedButCoherent:
      return PreparedContractOwnerClass::TargetUnsupportedButCoherent;
    case PreparedSelectedObjectDataContractStatus::
        InvalidPrePreparedInitializerSemantics:
      return PreparedContractOwnerClass::PrePreparedSemanticFailure;
  }
  return PreparedContractOwnerClass::ProducerIncoherent;
}

[[nodiscard]] PreparedContractOwnerClass
owner_for_frame_slot_address_source_route_status(
    PreparedFrameSlotAddressSourceRouteContractStatus status) {
  switch (status) {
    case PreparedFrameSlotAddressSourceRouteContractStatus::Coherent:
      return PreparedContractOwnerClass::Coherent;
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingRoute:
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingSourceSlot:
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingStackOffset:
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingExtent:
    case PreparedFrameSlotAddressSourceRouteContractStatus::MissingAlignment:
      return PreparedContractOwnerClass::ProducerMissing;
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingSourceHomeKind:
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        PartialAddressMaterialization:
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingMaterializationFrameSlot:
    case PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingCrossRoutePayload:
      return PreparedContractOwnerClass::ProducerIncoherent;
  }
  return PreparedContractOwnerClass::ProducerIncoherent;
}

[[nodiscard]] PreparedContractOwnerClass
owner_for_frame_slot_value_source_route_status(
    PreparedFrameSlotValueSourceRouteContractStatus status) {
  switch (status) {
    case PreparedFrameSlotValueSourceRouteContractStatus::Coherent:
      return PreparedContractOwnerClass::Coherent;
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingRoute:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceValueId:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceValueName:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceHomeKind:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceSlot:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingStackOffset:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingExtent:
    case PreparedFrameSlotValueSourceRouteContractStatus::MissingAlignment:
      return PreparedContractOwnerClass::ProducerMissing;
    case PreparedFrameSlotValueSourceRouteContractStatus::ConflictingSourceHomeKind:
    case PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingAddressMaterializationPayload:
    case PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingCrossRoutePayload:
      return PreparedContractOwnerClass::ProducerIncoherent;
  }
  return PreparedContractOwnerClass::ProducerIncoherent;
}

[[nodiscard]] PreparedContractFactFamily fact_family_for_selected_local_storage_status(
    PreparedSelectedLocalStorageContractStatus status) {
  switch (status) {
    case PreparedSelectedLocalStorageContractStatus::MissingExtent:
    case PreparedSelectedLocalStorageContractStatus::ConflictingExtent:
      return PreparedContractFactFamily::StorageObjectExtent;
    case PreparedSelectedLocalStorageContractStatus::MissingAlignment:
    case PreparedSelectedLocalStorageContractStatus::ConflictingAlignment:
      return PreparedContractFactFamily::StorageAlignment;
    case PreparedSelectedLocalStorageContractStatus::MissingByteRange:
    case PreparedSelectedLocalStorageContractStatus::ConflictingByteRange:
      return PreparedContractFactFamily::StorageByteRange;
    case PreparedSelectedLocalStorageContractStatus::MissingAliasAuthority:
    case PreparedSelectedLocalStorageContractStatus::ConflictingAliasAuthority:
      return PreparedContractFactFamily::StorageAliasAuthority;
    case PreparedSelectedLocalStorageContractStatus::MissingMemoryProvenance:
    case PreparedSelectedLocalStorageContractStatus::ConflictingMemoryProvenance:
      return PreparedContractFactFamily::MemoryAccessProvenance;
    case PreparedSelectedLocalStorageContractStatus::Coherent:
    case PreparedSelectedLocalStorageContractStatus::UnsupportedButCoherent:
      return PreparedContractFactFamily::StorageByteRange;
  }
  return PreparedContractFactFamily::StorageByteRange;
}

[[nodiscard]] PreparedContractFactFamily fact_family_for_selected_object_data_status(
    PreparedSelectedObjectDataContractStatus status) {
  switch (status) {
    case PreparedSelectedObjectDataContractStatus::MissingObjectLabel:
    case PreparedSelectedObjectDataContractStatus::ConflictingObjectLabel:
      return PreparedContractFactFamily::ObjectLabel;
    case PreparedSelectedObjectDataContractStatus::MissingPublicationIdentity:
    case PreparedSelectedObjectDataContractStatus::ConflictingPublicationIdentity:
      return PreparedContractFactFamily::ObjectPublicationIdentity;
    case PreparedSelectedObjectDataContractStatus::MissingEmittedBytes:
    case PreparedSelectedObjectDataContractStatus::ConflictingEmittedBytes:
    case PreparedSelectedObjectDataContractStatus::
        InvalidPrePreparedInitializerSemantics:
      return PreparedContractFactFamily::ObjectEmittedBytes;
    case PreparedSelectedObjectDataContractStatus::MissingZeroFill:
    case PreparedSelectedObjectDataContractStatus::ConflictingZeroFill:
      return PreparedContractFactFamily::ObjectZeroFill;
    case PreparedSelectedObjectDataContractStatus::MissingRelocation:
    case PreparedSelectedObjectDataContractStatus::ConflictingRelocation:
      return PreparedContractFactFamily::ObjectRelocation;
    case PreparedSelectedObjectDataContractStatus::MissingObjectByteRange:
    case PreparedSelectedObjectDataContractStatus::ConflictingObjectByteRange:
    case PreparedSelectedObjectDataContractStatus::Coherent:
      return PreparedContractFactFamily::ObjectByteRange;
    case PreparedSelectedObjectDataContractStatus::MissingUnsupportedObjectDataMarker:
    case PreparedSelectedObjectDataContractStatus::
        ConflictingUnsupportedObjectDataMarker:
    case PreparedSelectedObjectDataContractStatus::UnsupportedButCoherent:
      return PreparedContractFactFamily::UnsupportedObjectDataMarker;
  }
  return PreparedContractFactFamily::ObjectByteRange;
}

[[nodiscard]] constexpr std::string_view prepared_move_destination_kind_detail_name(
    PreparedMoveDestinationKind kind) {
  switch (kind) {
    case PreparedMoveDestinationKind::Value:
      return "value";
    case PreparedMoveDestinationKind::CallArgumentAbi:
      return "call_argument_abi";
    case PreparedMoveDestinationKind::CallResultAbi:
      return "call_result_abi";
    case PreparedMoveDestinationKind::FunctionReturnAbi:
      return "function_return_abi";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view prepared_move_storage_kind_detail_name(
    PreparedMoveStorageKind kind) {
  switch (kind) {
    case PreparedMoveStorageKind::None:
      return "none";
    case PreparedMoveStorageKind::Register:
      return "register";
    case PreparedMoveStorageKind::StackSlot:
      return "stack_slot";
  }
  return "unknown";
}

[[nodiscard]] std::string call_boundary_detail(
    const PreparedCallBoundaryMoveClassification& classification) {
  std::ostringstream out;
  out << "prepared call-boundary move classification status="
      << prepared_call_boundary_move_classification_status_name(
             classification.status);
  out << " phase=" << prepared_move_phase_name(classification.phase);
  out << " destination_kind="
      << prepared_move_destination_kind_detail_name(classification.destination_kind);
  out << " storage_kind="
      << prepared_move_storage_kind_detail_name(classification.storage_kind);
  if (classification.abi_index.has_value()) {
    out << " abi_index=" << *classification.abi_index;
  }
  return out.str();
}

[[nodiscard]] std::string variadic_entry_missing_detail(
    const PreparedVariadicEntryPlanFunction& entry_plan) {
  std::ostringstream out;
  out << "prepared variadic entry plan is incomplete";
  if (!entry_plan.missing_required_facts.empty()) {
    out << "; missing fact=" << entry_plan.missing_required_facts.front();
  }
  return out.str();
}

[[nodiscard]] std::string variadic_helper_homes_detail(
    const PreparedVariadicEntryHelperOperandHomes* homes,
    PreparedVariadicEntryHelperKind expected_helper,
    std::size_t block_index,
    std::size_t instruction_index) {
  std::ostringstream out;
  out << "prepared variadic helper operand-home/access-plan facts are incomplete";
  out << " helper=" << prepared_variadic_entry_helper_kind_name(expected_helper);
  out << " block_index=" << block_index;
  out << " instruction_index=" << instruction_index;
  if (homes != nullptr && homes->helper != expected_helper) {
    out << " actual_helper=" << prepared_variadic_entry_helper_kind_name(homes->helper);
  }
  return out.str();
}

[[nodiscard]] std::string selected_local_storage_detail(
    const PreparedSelectedLocalStorageContractFacts& facts,
    PreparedSelectedLocalStorageContractStatus status) {
  std::ostringstream out;
  out << "prepared selected local storage contract status="
      << prepared_selected_local_storage_contract_status_name(status);
  out << " function_name=" << facts.function_name;
  if (facts.object_id.has_value()) {
    out << " object_id=" << *facts.object_id;
  }
  if (facts.frame_slot_id.has_value()) {
    out << " frame_slot_id=" << *facts.frame_slot_id;
  }
  out << " byte_offset=" << facts.byte_offset;
  out << " size_bytes=" << facts.size_bytes;
  out << " align_bytes=" << facts.align_bytes;
  return out.str();
}

[[nodiscard]] std::string selected_object_data_detail(
    const PreparedSelectedObjectDataContractFacts& facts,
    PreparedSelectedObjectDataContractStatus status) {
  std::ostringstream out;
  out << "prepared selected object-data contract status="
      << prepared_selected_object_data_contract_status_name(status);
  if (facts.object_label.has_value()) {
    out << " object_label_id=" << *facts.object_label;
  }
  out << " object_size_bytes=" << facts.object_size_bytes;
  out << " emitted_byte_count=" << facts.emitted_byte_count;
  out << " zero_fill_byte_count=" << facts.zero_fill_byte_count;
  return out.str();
}

[[nodiscard]] bool frame_slot_address_selection_has_cross_route_payload(
    const PreparedCallArgumentSourceSelection& selection) {
  return selection.source_base_value_id.has_value() ||
         selection.source_pointer_byte_delta.has_value() ||
         prepared_call_argument_source_selection_has_preservation_payload(selection) ||
         selection.byval_lane_extent_bytes.has_value() ||
         selection.byval_lane_source_instruction_index.has_value();
}

[[nodiscard]] bool frame_slot_value_selection_has_cross_route_payload(
    const PreparedCallArgumentSourceSelection& selection) {
  return selection.source_base_value_id.has_value() ||
         selection.source_pointer_byte_delta.has_value() ||
         prepared_call_argument_source_selection_has_preservation_payload(selection) ||
         selection.byval_lane_extent_bytes.has_value() ||
         selection.byval_lane_source_instruction_index.has_value();
}

[[nodiscard]] std::string frame_slot_address_source_route_detail(
    const PreparedCallArgumentSourceSelection* selection,
    PreparedFrameSlotAddressSourceRouteContractStatus status) {
  std::ostringstream out;
  out << "prepared frame-slot address source route contract status="
      << prepared_frame_slot_address_source_route_contract_status_name(status);
  if (selection == nullptr) {
    return out.str();
  }
  out << " selection_kind="
      << prepared_call_argument_source_selection_kind_name(selection->kind);
  if (selection->source_value_id.has_value()) {
    out << " source_value_id=" << *selection->source_value_id;
  }
  if (selection->source_value_name.has_value()) {
    out << " source_value_name=" << *selection->source_value_name;
  }
  if (selection->source_slot_id.has_value()) {
    out << " source_slot_id=" << *selection->source_slot_id;
  }
  if (selection->source_stack_offset_bytes.has_value()) {
    out << " source_stack_offset_bytes="
        << *selection->source_stack_offset_bytes;
  }
  if (selection->source_size_bytes.has_value()) {
    out << " source_size_bytes=" << *selection->source_size_bytes;
  }
  if (selection->source_align_bytes.has_value()) {
    out << " source_align_bytes=" << *selection->source_align_bytes;
  }
  return out.str();
}

[[nodiscard]] std::string frame_slot_value_source_route_detail(
    const PreparedCallArgumentSourceSelection* selection,
    PreparedFrameSlotValueSourceRouteContractStatus status) {
  std::ostringstream out;
  out << "prepared frame-slot value source route contract status="
      << prepared_frame_slot_value_source_route_contract_status_name(status);
  if (selection == nullptr) {
    return out.str();
  }
  out << " selection_kind="
      << prepared_call_argument_source_selection_kind_name(selection->kind);
  if (selection->source_value_id.has_value()) {
    out << " source_value_id=" << *selection->source_value_id;
  }
  if (selection->source_value_name.has_value()) {
    out << " source_value_name=" << *selection->source_value_name;
  }
  if (selection->source_slot_id.has_value()) {
    out << " source_slot_id=" << *selection->source_slot_id;
  }
  if (selection->source_stack_offset_bytes.has_value()) {
    out << " source_stack_offset_bytes="
        << *selection->source_stack_offset_bytes;
  }
  if (selection->source_size_bytes.has_value()) {
    out << " source_size_bytes=" << *selection->source_size_bytes;
  }
  if (selection->source_align_bytes.has_value()) {
    out << " source_align_bytes=" << *selection->source_align_bytes;
  }
  return out.str();
}

}  // namespace

PreparedContractVerificationReport verify_prepared_decoded_home_storage_contract(
    const PreparedDecodedHomeStorage& decoded) {
  const auto owner = owner_for_decoded_home_storage_status(decoded);
  if (owner == PreparedContractOwnerClass::Coherent) {
    return PreparedContractVerificationReport{
        .fact_family = PreparedContractFactFamily::ValueHomeTypedStorage,
        .owner_class = owner,
        .function_name = decoded.function_name,
        .value_id = decoded.value_id,
        .value_name = decoded.value_name,
        .fail_closed = false,
    };
  }

  const auto diagnostic = build_prepared_decoded_home_storage_diagnostic(decoded);
  return PreparedContractVerificationReport{
      .fact_family = PreparedContractFactFamily::ValueHomeTypedStorage,
      .owner_class = owner,
      .function_name = diagnostic.function_name,
      .value_id = diagnostic.value_id,
      .value_name = diagnostic.value_name,
      .fail_closed = owner != PreparedContractOwnerClass::Coherent,
      .detail = diagnostic.message,
  };
}

PreparedContractVerificationReport verify_prepared_call_boundary_move_contract(
    const PreparedCallBoundaryMoveClassification& classification) {
  const auto owner = owner_for_call_boundary_status(classification.status);
  const auto value_id = classification.move != nullptr
                            ? classification.move->from_value_id
                            : PreparedValueId{0};
  return PreparedContractVerificationReport{
      .fact_family =
          PreparedContractFactFamily::CallBoundaryArgumentResultPlan,
      .owner_class = owner,
      .value_id = value_id,
      .fail_closed = owner != PreparedContractOwnerClass::Coherent,
      .detail = owner == PreparedContractOwnerClass::Coherent
                    ? std::string{}
                    : call_boundary_detail(classification),
  };
}

PreparedContractVerificationReport verify_prepared_variadic_entry_plan_contract(
    const PreparedVariadicEntryPlanFunction* entry_plan,
    FunctionNameId function_name) {
  if (entry_plan == nullptr) {
    return PreparedContractVerificationReport{
        .fact_family =
            PreparedContractFactFamily::VariadicEntryHelperOperandHomes,
        .owner_class = PreparedContractOwnerClass::ProducerMissing,
        .function_name = function_name,
        .fail_closed = true,
        .detail = "missing prepared variadic entry plan",
    };
  }
  if (!entry_plan->missing_required_facts.empty()) {
    return PreparedContractVerificationReport{
        .fact_family =
            PreparedContractFactFamily::VariadicEntryHelperOperandHomes,
        .owner_class = PreparedContractOwnerClass::ProducerMissing,
        .function_name = entry_plan->function_name,
        .fail_closed = true,
        .detail = variadic_entry_missing_detail(*entry_plan),
    };
  }
  return PreparedContractVerificationReport{
      .fact_family =
          PreparedContractFactFamily::VariadicEntryHelperOperandHomes,
      .owner_class = PreparedContractOwnerClass::Coherent,
      .function_name = entry_plan->function_name,
      .fail_closed = false,
  };
}

PreparedContractVerificationReport
verify_prepared_variadic_entry_helper_operand_homes_contract(
    const PreparedVariadicEntryHelperOperandHomes* homes,
    FunctionNameId function_name,
    PreparedVariadicEntryHelperKind expected_helper,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (homes == nullptr) {
    return PreparedContractVerificationReport{
        .fact_family =
            PreparedContractFactFamily::VariadicEntryHelperOperandHomes,
        .owner_class = PreparedContractOwnerClass::ProducerMissing,
        .function_name = function_name,
        .fail_closed = true,
        .detail = variadic_helper_homes_detail(
            homes, expected_helper, block_index, instruction_index),
    };
  }
  const bool coherent =
      homes->helper == expected_helper &&
      has_complete_prepared_variadic_entry_helper_operand_homes(*homes);
  return PreparedContractVerificationReport{
      .fact_family =
          PreparedContractFactFamily::VariadicEntryHelperOperandHomes,
      .owner_class = coherent ? PreparedContractOwnerClass::Coherent
                              : PreparedContractOwnerClass::ProducerIncoherent,
      .function_name = function_name,
      .fail_closed = !coherent,
      .detail = coherent ? std::string{}
                         : variadic_helper_homes_detail(
                               homes, expected_helper, block_index, instruction_index),
  };
}

PreparedSelectedLocalStorageContractStatus
classify_prepared_selected_local_storage_contract(
    const PreparedSelectedLocalStorageContractFacts& facts) {
  if (!facts.has_extent || facts.size_bytes == 0) {
    return PreparedSelectedLocalStorageContractStatus::MissingExtent;
  }
  if (!facts.has_alignment || facts.align_bytes == 0) {
    return PreparedSelectedLocalStorageContractStatus::MissingAlignment;
  }
  if (!facts.has_byte_range) {
    return PreparedSelectedLocalStorageContractStatus::MissingByteRange;
  }
  if (facts.alias_authority_required && !facts.has_alias_authority) {
    return PreparedSelectedLocalStorageContractStatus::MissingAliasAuthority;
  }
  if (!facts.has_memory_provenance) {
    return PreparedSelectedLocalStorageContractStatus::MissingMemoryProvenance;
  }
  if (facts.conflicting_extent) {
    return PreparedSelectedLocalStorageContractStatus::ConflictingExtent;
  }
  if (facts.conflicting_alignment) {
    return PreparedSelectedLocalStorageContractStatus::ConflictingAlignment;
  }
  if (facts.conflicting_byte_range) {
    return PreparedSelectedLocalStorageContractStatus::ConflictingByteRange;
  }
  if (facts.conflicting_alias_authority) {
    return PreparedSelectedLocalStorageContractStatus::ConflictingAliasAuthority;
  }
  if (facts.conflicting_memory_provenance) {
    return PreparedSelectedLocalStorageContractStatus::ConflictingMemoryProvenance;
  }
  if (facts.unsupported_but_coherent) {
    return PreparedSelectedLocalStorageContractStatus::UnsupportedButCoherent;
  }
  return PreparedSelectedLocalStorageContractStatus::Coherent;
}

PreparedContractVerificationReport verify_prepared_selected_local_storage_contract(
    const PreparedSelectedLocalStorageContractFacts& facts) {
  const auto status = classify_prepared_selected_local_storage_contract(facts);
  const auto owner = owner_for_selected_local_storage_status(status);
  return PreparedContractVerificationReport{
      .fact_family = fact_family_for_selected_local_storage_status(status),
      .owner_class = owner,
      .function_name = facts.function_name,
      .fail_closed = owner != PreparedContractOwnerClass::Coherent,
      .detail = owner == PreparedContractOwnerClass::Coherent
                    ? std::string{}
                    : selected_local_storage_detail(facts, status),
  };
}

PreparedSelectedObjectDataContractStatus
classify_prepared_selected_object_data_contract(
    const PreparedSelectedObjectDataContractFacts& facts) {
  if (facts.invalid_pre_prepared_initializer_semantics) {
    return PreparedSelectedObjectDataContractStatus::
        InvalidPrePreparedInitializerSemantics;
  }
  if (!facts.has_object_label) {
    return PreparedSelectedObjectDataContractStatus::MissingObjectLabel;
  }
  if (!facts.has_publication_identity) {
    return PreparedSelectedObjectDataContractStatus::MissingPublicationIdentity;
  }
  if (facts.requires_emitted_bytes && !facts.has_emitted_bytes) {
    return PreparedSelectedObjectDataContractStatus::MissingEmittedBytes;
  }
  if (facts.requires_zero_fill && !facts.has_zero_fill) {
    return PreparedSelectedObjectDataContractStatus::MissingZeroFill;
  }
  if (facts.requires_relocation && !facts.has_relocation) {
    return PreparedSelectedObjectDataContractStatus::MissingRelocation;
  }
  if (!facts.has_object_byte_range || facts.object_size_bytes == 0) {
    return PreparedSelectedObjectDataContractStatus::MissingObjectByteRange;
  }
  if (facts.requires_unsupported_marker && !facts.has_unsupported_marker) {
    return PreparedSelectedObjectDataContractStatus::
        MissingUnsupportedObjectDataMarker;
  }
  if (facts.conflicting_object_label) {
    return PreparedSelectedObjectDataContractStatus::ConflictingObjectLabel;
  }
  if (facts.conflicting_publication_identity) {
    return PreparedSelectedObjectDataContractStatus::
        ConflictingPublicationIdentity;
  }
  if (facts.conflicting_emitted_bytes) {
    return PreparedSelectedObjectDataContractStatus::ConflictingEmittedBytes;
  }
  if (facts.conflicting_zero_fill) {
    return PreparedSelectedObjectDataContractStatus::ConflictingZeroFill;
  }
  if (facts.conflicting_relocation) {
    return PreparedSelectedObjectDataContractStatus::ConflictingRelocation;
  }
  if (facts.conflicting_object_byte_range) {
    return PreparedSelectedObjectDataContractStatus::ConflictingObjectByteRange;
  }
  if (facts.conflicting_unsupported_marker) {
    return PreparedSelectedObjectDataContractStatus::
        ConflictingUnsupportedObjectDataMarker;
  }
  if (facts.unsupported_but_coherent) {
    return PreparedSelectedObjectDataContractStatus::UnsupportedButCoherent;
  }
  return PreparedSelectedObjectDataContractStatus::Coherent;
}

PreparedContractVerificationReport verify_prepared_selected_object_data_contract(
    const PreparedSelectedObjectDataContractFacts& facts) {
  const auto status = classify_prepared_selected_object_data_contract(facts);
  const auto owner = owner_for_selected_object_data_status(status);
  return PreparedContractVerificationReport{
      .fact_family = fact_family_for_selected_object_data_status(status),
      .owner_class = owner,
      .fail_closed = owner != PreparedContractOwnerClass::Coherent,
      .detail = owner == PreparedContractOwnerClass::Coherent
                    ? std::string{}
                    : selected_object_data_detail(facts, status),
  };
}

PreparedFrameSlotAddressSourceRouteContractStatus
classify_prepared_frame_slot_address_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection) {
  if (selection == nullptr ||
      selection->kind != PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::MissingRoute;
  }
  if (!selection->source_slot_id.has_value()) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::MissingSourceSlot;
  }
  if (!selection->source_stack_offset_bytes.has_value()) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::MissingStackOffset;
  }
  if (!selection->source_size_bytes.has_value()) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::MissingExtent;
  }
  if (!selection->source_align_bytes.has_value()) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::MissingAlignment;
  }
  if (selection->source_home_kind.has_value() &&
      *selection->source_home_kind != PreparedValueHomeKind::StackSlot) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingSourceHomeKind;
  }

  const bool has_any_materialization =
      selection->address_materialization_block_label.has_value() ||
      selection->address_materialization_inst_index.has_value() ||
      selection->address_materialization_frame_slot_id.has_value() ||
      selection->address_materialization_byte_offset.has_value();
  const bool has_complete_materialization =
      selection->address_materialization_block_label.has_value() &&
      selection->address_materialization_inst_index.has_value() &&
      selection->address_materialization_frame_slot_id.has_value() &&
      selection->address_materialization_byte_offset.has_value();
  if (has_any_materialization && !has_complete_materialization) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::
        PartialAddressMaterialization;
  }
  if (has_complete_materialization &&
      *selection->address_materialization_frame_slot_id !=
          *selection->source_slot_id) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingMaterializationFrameSlot;
  }
  if (frame_slot_address_selection_has_cross_route_payload(*selection)) {
    return PreparedFrameSlotAddressSourceRouteContractStatus::
        ConflictingCrossRoutePayload;
  }
  return PreparedFrameSlotAddressSourceRouteContractStatus::Coherent;
}

PreparedContractVerificationReport
verify_prepared_frame_slot_address_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection) {
  const auto status =
      classify_prepared_frame_slot_address_source_route_contract(selection);
  const auto owner = owner_for_frame_slot_address_source_route_status(status);
  return PreparedContractVerificationReport{
      .fact_family = PreparedContractFactFamily::CallArgumentTypedRoute,
      .owner_class = owner,
      .value_id = selection != nullptr && selection->source_value_id.has_value()
                      ? *selection->source_value_id
                      : PreparedValueId{0},
      .value_name = selection != nullptr && selection->source_value_name.has_value()
                        ? *selection->source_value_name
                        : kInvalidValueName,
      .fail_closed = owner != PreparedContractOwnerClass::Coherent,
      .detail = owner == PreparedContractOwnerClass::Coherent
                    ? std::string{}
                    : frame_slot_address_source_route_detail(selection, status),
  };
}

PreparedFrameSlotValueSourceRouteContractStatus
classify_prepared_frame_slot_value_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection) {
  if (selection == nullptr ||
      selection->kind != PreparedCallArgumentSourceSelectionKind::FrameSlotValue) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingRoute;
  }
  if (!selection->source_value_id.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceValueId;
  }
  if (!selection->source_value_name.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceValueName;
  }
  if (!selection->source_home_kind.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceHomeKind;
  }
  if (*selection->source_home_kind != PreparedValueHomeKind::StackSlot) {
    return PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingSourceHomeKind;
  }
  if (!selection->source_slot_id.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingSourceSlot;
  }
  if (!selection->source_stack_offset_bytes.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingStackOffset;
  }
  if (!selection->source_size_bytes.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingExtent;
  }
  if (!selection->source_align_bytes.has_value()) {
    return PreparedFrameSlotValueSourceRouteContractStatus::MissingAlignment;
  }
  if (prepared_call_argument_source_selection_has_address_materialization_payload(
          *selection)) {
    return PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingAddressMaterializationPayload;
  }
  if (frame_slot_value_selection_has_cross_route_payload(*selection)) {
    return PreparedFrameSlotValueSourceRouteContractStatus::
        ConflictingCrossRoutePayload;
  }
  return PreparedFrameSlotValueSourceRouteContractStatus::Coherent;
}

PreparedContractVerificationReport
verify_prepared_frame_slot_value_source_route_contract(
    const PreparedCallArgumentSourceSelection* selection) {
  const auto status =
      classify_prepared_frame_slot_value_source_route_contract(selection);
  const auto owner = owner_for_frame_slot_value_source_route_status(status);
  return PreparedContractVerificationReport{
      .fact_family = PreparedContractFactFamily::CallArgumentTypedRoute,
      .owner_class = owner,
      .value_id = selection != nullptr && selection->source_value_id.has_value()
                      ? *selection->source_value_id
                      : PreparedValueId{0},
      .value_name = selection != nullptr && selection->source_value_name.has_value()
                        ? *selection->source_value_name
                        : kInvalidValueName,
      .fail_closed = owner != PreparedContractOwnerClass::Coherent,
      .detail = owner == PreparedContractOwnerClass::Coherent
                    ? std::string{}
                    : frame_slot_value_source_route_detail(selection, status),
  };
}

}  // namespace c4c::backend::prepare
