#include "src/backend/prealloc/prepared_contract_verifier.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

namespace prepare = c4c::backend::prepare;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

prepare::PreparedSelectedLocalStorageContractFacts coherent_local_storage() {
  return prepare::PreparedSelectedLocalStorageContractFacts{
      .function_name = c4c::FunctionNameId{7},
      .object_id = prepare::PreparedObjectId{3},
      .frame_slot_id = prepare::PreparedFrameSlotId{11},
      .byte_offset = 16,
      .size_bytes = 8,
      .align_bytes = 8,
      .has_extent = true,
      .has_alignment = true,
      .has_byte_range = true,
      .alias_authority_required = true,
      .has_alias_authority = true,
      .has_memory_provenance = true,
  };
}

prepare::PreparedSelectedObjectDataContractFacts coherent_object_data() {
  return prepare::PreparedSelectedObjectDataContractFacts{
      .object_label = c4c::LinkNameId{5},
      .object_size_bytes = 16,
      .emitted_byte_count = 8,
      .zero_fill_byte_count = 8,
      .has_object_label = true,
      .has_publication_identity = true,
      .requires_emitted_bytes = true,
      .has_emitted_bytes = true,
      .requires_zero_fill = true,
      .has_zero_fill = true,
      .requires_relocation = true,
      .has_relocation = true,
      .has_object_byte_range = true,
  };
}

prepare::PreparedCallArgumentSourceSelection coherent_frame_slot_address_route() {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
      .source_value_id = prepare::PreparedValueId{19},
      .source_value_name = c4c::ValueNameId{23},
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .source_slot_id = prepare::PreparedFrameSlotId{29},
      .source_stack_offset_bytes = std::size_t{32},
      .source_size_bytes = std::size_t{8},
      .source_align_bytes = std::size_t{8},
      .address_materialization_block_label = c4c::BlockLabelId{31},
      .address_materialization_inst_index = std::size_t{5},
      .address_materialization_frame_slot_id = prepare::PreparedFrameSlotId{29},
      .address_materialization_byte_offset = std::int64_t{32},
  };
}

prepare::PreparedCallArgumentSourceSelection coherent_frame_slot_value_route() {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
      .source_value_id = prepare::PreparedValueId{37},
      .source_value_name = c4c::ValueNameId{41},
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .source_slot_id = prepare::PreparedFrameSlotId{43},
      .source_stack_offset_bytes = std::size_t{48},
      .source_size_bytes = std::size_t{4},
      .source_align_bytes = std::size_t{4},
  };
}

prepare::PreparedCallArgumentSourceSelection
coherent_local_frame_address_materialization_route() {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization,
      .source_value_id = prepare::PreparedValueId{53},
      .source_value_name = c4c::ValueNameId{59},
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
      .source_slot_id = prepare::PreparedFrameSlotId{61},
      .source_stack_offset_bytes = std::size_t{64},
      .source_size_bytes = std::size_t{16},
      .source_align_bytes = std::size_t{8},
      .source_pointer_byte_delta = std::int64_t{0},
      .address_materialization_block_label = c4c::BlockLabelId{67},
      .address_materialization_inst_index = std::size_t{3},
      .address_materialization_frame_slot_id = prepare::PreparedFrameSlotId{61},
      .address_materialization_byte_offset = std::int64_t{64},
  };
}

int verify_selected_local_storage_contract_reports() {
  const auto coherent =
      prepare::verify_prepared_selected_local_storage_contract(
          coherent_local_storage());

  auto missing = coherent_local_storage();
  missing.has_extent = false;
  const auto missing_report =
      prepare::verify_prepared_selected_local_storage_contract(missing);

  auto incoherent = coherent_local_storage();
  incoherent.conflicting_byte_range = true;
  const auto incoherent_report =
      prepare::verify_prepared_selected_local_storage_contract(incoherent);

  auto unsupported = coherent_local_storage();
  unsupported.unsupported_but_coherent = true;
  const auto unsupported_report =
      prepare::verify_prepared_selected_local_storage_contract(unsupported);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete local storage facts should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent local storage facts should not fail closed") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing extent should classify as producer missing") ||
      !expect(missing_report.fact_family ==
                  prepare::PreparedContractFactFamily::StorageObjectExtent,
              "missing extent should identify storage object extent family") ||
      !expect(incoherent_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "conflicting byte range should classify as producer incoherent") ||
      !expect(incoherent_report.fact_family ==
                  prepare::PreparedContractFactFamily::StorageByteRange,
              "conflicting byte range should identify storage byte range family") ||
      !expect(unsupported_report.owner_class ==
                  prepare::PreparedContractOwnerClass::
                      TargetUnsupportedButCoherent,
              "complete unsupported storage form should classify as target unsupported") ||
      !expect(prepare::prepared_contract_fact_family_name(
                  prepare::PreparedContractFactFamily::MemoryAccessProvenance) ==
                  std::string_view{"memory_access_provenance"},
              "memory provenance family spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_selected_object_data_contract_reports() {
  const auto coherent =
      prepare::verify_prepared_selected_object_data_contract(coherent_object_data());

  auto missing = coherent_object_data();
  missing.has_relocation = false;
  const auto missing_report =
      prepare::verify_prepared_selected_object_data_contract(missing);

  auto incoherent = coherent_object_data();
  incoherent.conflicting_zero_fill = true;
  const auto incoherent_report =
      prepare::verify_prepared_selected_object_data_contract(incoherent);

  auto unsupported = coherent_object_data();
  unsupported.requires_unsupported_marker = true;
  unsupported.has_unsupported_marker = true;
  unsupported.unsupported_but_coherent = true;
  const auto unsupported_report =
      prepare::verify_prepared_selected_object_data_contract(unsupported);

  auto semantic_failure = coherent_object_data();
  semantic_failure.invalid_pre_prepared_initializer_semantics = true;
  const auto semantic_failure_report =
      prepare::verify_prepared_selected_object_data_contract(semantic_failure);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete object data facts should be coherent") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing relocation should classify as producer missing") ||
      !expect(missing_report.fact_family ==
                  prepare::PreparedContractFactFamily::ObjectRelocation,
              "missing relocation should identify object relocation family") ||
      !expect(incoherent_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "conflicting zero-fill should classify as producer incoherent") ||
      !expect(incoherent_report.fact_family ==
                  prepare::PreparedContractFactFamily::ObjectZeroFill,
              "conflicting zero-fill should identify object zero-fill family") ||
      !expect(unsupported_report.owner_class ==
                  prepare::PreparedContractOwnerClass::
                      TargetUnsupportedButCoherent,
              "complete unsupported object form should classify as target unsupported") ||
      !expect(unsupported_report.fact_family ==
                  prepare::PreparedContractFactFamily::UnsupportedObjectDataMarker,
              "unsupported object form should identify marker family") ||
      !expect(semantic_failure_report.owner_class ==
                  prepare::PreparedContractOwnerClass::
                      PrePreparedSemanticFailure,
              "invalid initializer semantics should classify before prepared") ||
      !expect(prepare::prepared_selected_object_data_contract_status_name(
                  prepare::PreparedSelectedObjectDataContractStatus::
                      InvalidPrePreparedInitializerSemantics) ==
                  std::string_view{
                      "invalid_pre_prepared_initializer_semantics"},
              "semantic failure status spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_frame_slot_address_source_route_contract_reports() {
  auto coherent_route = coherent_frame_slot_address_route();
  const auto coherent =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &coherent_route);

  auto missing = coherent_frame_slot_address_route();
  missing.source_stack_offset_bytes = std::nullopt;
  const auto missing_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &missing);

  auto incoherent = coherent_frame_slot_address_route();
  incoherent.address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{30};
  const auto incoherent_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &incoherent);

  auto cross_route = coherent_frame_slot_address_route();
  cross_route.byval_lane_extent_bytes = std::size_t{8};
  const auto cross_route_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &cross_route);

  const auto missing_route_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(nullptr);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete frame-slot address route should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent frame-slot address route should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute,
              "frame-slot address route should identify typed route family") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing frame-slot address stack offset should classify as producer missing") ||
      !expect(missing_report.fail_closed,
              "missing frame-slot address route fact should fail closed") ||
      !expect(prepare::classify_prepared_frame_slot_address_source_route_contract(
                  &missing) ==
                  prepare::PreparedFrameSlotAddressSourceRouteContractStatus::
                      MissingStackOffset,
              "missing stack offset should have precise route status") ||
      !expect(incoherent_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory materialization slot should classify as producer incoherent") ||
      !expect(cross_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-route payload should classify as producer incoherent") ||
      !expect(missing_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "absent frame-slot address route should classify as producer missing") ||
      !expect(prepare::prepared_contract_fact_family_name(
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute) ==
                  std::string_view{"call_argument_typed_route"},
              "typed call-argument route family spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_frame_slot_value_source_route_contract_reports() {
  auto coherent_route = coherent_frame_slot_value_route();
  const auto coherent =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &coherent_route);

  auto missing = coherent_frame_slot_value_route();
  missing.source_value_name = std::nullopt;
  const auto missing_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &missing);

  auto wrong_home = coherent_frame_slot_value_route();
  wrong_home.source_home_kind = prepare::PreparedValueHomeKind::Register;
  const auto wrong_home_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &wrong_home);

  auto address_payload = coherent_frame_slot_value_route();
  address_payload.address_materialization_block_label = c4c::BlockLabelId{5};
  const auto address_payload_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &address_payload);

  auto cross_route = coherent_frame_slot_value_route();
  cross_route.byval_lane_extent_bytes = std::size_t{4};
  const auto cross_route_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &cross_route);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete frame-slot value route should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent frame-slot value route should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute,
              "frame-slot value route should identify typed route family") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing frame-slot value identity should classify as producer missing") ||
      !expect(prepare::classify_prepared_frame_slot_value_source_route_contract(
                  &missing) ==
                  prepare::PreparedFrameSlotValueSourceRouteContractStatus::
                      MissingSourceValueName,
              "missing value name should have precise route status") ||
      !expect(wrong_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong frame-slot value home kind should classify as producer incoherent") ||
      !expect(address_payload_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "address materialization payload should classify as producer incoherent") ||
      !expect(cross_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-route frame-slot value payload should classify as producer incoherent") ||
      !expect(prepare::prepared_frame_slot_value_source_route_contract_status_name(
                  prepare::PreparedFrameSlotValueSourceRouteContractStatus::
                      ConflictingAddressMaterializationPayload) ==
                  std::string_view{"conflicting_address_materialization_payload"},
              "frame-slot value materialization status spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_local_frame_address_materialization_source_route_contract_reports() {
  auto coherent_route = coherent_local_frame_address_materialization_route();
  const auto coherent =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &coherent_route);

  auto missing_delta = coherent_local_frame_address_materialization_route();
  missing_delta.source_pointer_byte_delta = std::nullopt;
  const auto missing_delta_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &missing_delta);

  auto missing_base = coherent_local_frame_address_materialization_route();
  missing_base.source_home_kind =
      prepare::PreparedValueHomeKind::PointerBasePlusOffset;
  missing_base.source_base_value_id = std::nullopt;
  const auto missing_base_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &missing_base);

  auto wrong_home = coherent_local_frame_address_materialization_route();
  wrong_home.source_home_kind = prepare::PreparedValueHomeKind::StackSlot;
  const auto wrong_home_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &wrong_home);

  auto missing_location = coherent_local_frame_address_materialization_route();
  missing_location.address_materialization_block_label = std::nullopt;
  const auto missing_location_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &missing_location);

  auto mismatched_slot = coherent_local_frame_address_materialization_route();
  mismatched_slot.address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{62};
  const auto mismatched_slot_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &mismatched_slot);

  auto mismatched_offset = coherent_local_frame_address_materialization_route();
  mismatched_offset.address_materialization_byte_offset = std::int64_t{63};
  const auto mismatched_offset_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &mismatched_offset);

  auto cross_route = coherent_local_frame_address_materialization_route();
  cross_route.byval_lane_extent_bytes = std::size_t{8};
  const auto cross_route_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &cross_route);

  const auto missing_route_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              nullptr);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete local materialization route should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent local materialization route should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute,
              "local materialization route should identify typed route family") ||
      !expect(missing_delta_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer delta should classify as producer missing") ||
      !expect(prepare::
                  classify_prepared_local_frame_address_materialization_source_route_contract(
                      &missing_delta) ==
                  prepare::
                      PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
                          MissingPointerByteDelta,
              "missing pointer delta should have precise route status") ||
      !expect(missing_base_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer base identity should classify as producer missing") ||
      !expect(prepare::
                  classify_prepared_local_frame_address_materialization_source_route_contract(
                      &missing_base) ==
                  prepare::
                      PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
                          MissingSourceBaseValueId,
              "missing pointer base identity should have precise route status") ||
      !expect(wrong_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong local materialization home kind should classify as producer incoherent") ||
      !expect(missing_location_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing materialization location should classify as producer missing") ||
      !expect(mismatched_slot_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory local materialization slot should classify as producer incoherent") ||
      !expect(mismatched_offset_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory local materialization offset should classify as producer incoherent") ||
      !expect(cross_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-route local materialization payload should classify as producer incoherent") ||
      !expect(missing_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "absent local materialization route should classify as producer missing") ||
      !expect(prepare::
                  prepared_local_frame_address_materialization_source_route_contract_status_name(
                      prepare::
                          PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
                              ConflictingMaterializationByteOffset) ==
                  std::string_view{"conflicting_materialization_byte_offset"},
              "local materialization offset status spelling mismatch")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (const int rc = verify_selected_local_storage_contract_reports(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_selected_object_data_contract_reports(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_frame_slot_address_source_route_contract_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc = verify_frame_slot_value_source_route_contract_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          verify_local_frame_address_materialization_source_route_contract_reports();
      rc != 0) {
    return rc;
  }
  return EXIT_SUCCESS;
}
