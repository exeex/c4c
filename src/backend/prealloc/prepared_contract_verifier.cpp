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

}  // namespace c4c::backend::prepare
