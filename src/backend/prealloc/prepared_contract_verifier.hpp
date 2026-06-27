#pragma once

#include "decoded_home_storage.hpp"

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
  CallBoundaryArgumentResultPlan,
  VariadicEntryHelperOperandHomes,
};

[[nodiscard]] constexpr std::string_view prepared_contract_fact_family_name(
    PreparedContractFactFamily family) {
  switch (family) {
    case PreparedContractFactFamily::ValueHomeTypedStorage:
      return "value_home_typed_storage";
    case PreparedContractFactFamily::CallBoundaryArgumentResultPlan:
      return "call_boundary_argument_result_plan";
    case PreparedContractFactFamily::VariadicEntryHelperOperandHomes:
      return "variadic_entry_helper_operand_homes";
  }
  return "unknown";
}

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

}  // namespace c4c::backend::prepare
