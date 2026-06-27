#include "src/backend/prealloc/prepared_contract_verifier.hpp"

#include <cstdlib>
#include <iostream>
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

}  // namespace

int main() {
  if (const int rc = verify_selected_local_storage_contract_reports(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_selected_object_data_contract_reports(); rc != 0) {
    return rc;
  }
  return EXIT_SUCCESS;
}
