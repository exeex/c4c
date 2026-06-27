#include "prepared_contract_verifier.hpp"

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

}  // namespace c4c::backend::prepare
