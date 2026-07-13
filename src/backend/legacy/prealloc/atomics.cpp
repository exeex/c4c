#include "atomics.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::prepare {

namespace {

void append_atomic_missing_fact(PreparedAtomicOperationFunction& function_operations,
                                PreparedAtomicOperationCarrier& carrier,
                                std::string fact) {
  carrier.missing_required_facts.push_back(fact);
  function_operations.missing_required_facts.push_back(
      "inst#" + std::to_string(carrier.inst_index) + ":" + std::move(fact));
}

[[nodiscard]] std::optional<ValueNameId> prepared_atomic_named_value_id(
    PreparedNameTables& names,
    const std::optional<bir::Value>& value) {
  if (!value.has_value()) {
    return std::nullopt;
  }
  return prepared_named_value_id(names, *value);
}

[[nodiscard]] PreparedAtomicOperationCarrier build_atomic_operation_carrier(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    PreparedAtomicOperationFunction& function_operations,
    const bir::AtomicOperation& operation) {
  const std::string_view block_label =
      operation.block_label_id == kInvalidBlockLabel
          ? std::string_view{operation.block_label}
          : bir_names.block_labels.spelling(operation.block_label_id);
  PreparedAtomicOperationCarrier carrier{
      .function_name = function_operations.function_name,
      .carrier_kind = PreparedAtomicOperationCarrierKind::Missing,
      .operation_kind = operation.kind,
      .block_label = names.block_labels.intern(
          block_label.empty() ? std::string_view{operation.block_label} : block_label),
      .inst_index = operation.inst_index,
      .value_type = operation.value_type,
      .width_bytes = operation.width_bytes,
      .result = operation.result,
      .pointer = operation.pointer,
      .value = operation.value,
      .expected = operation.expected,
      .desired = operation.desired,
      .result_value_name = prepared_atomic_named_value_id(names, operation.result),
      .pointer_value_name = prepared_atomic_named_value_id(names, operation.pointer),
      .value_name = prepared_atomic_named_value_id(names, operation.value),
      .expected_value_name = prepared_atomic_named_value_id(names, operation.expected),
      .desired_value_name = prepared_atomic_named_value_id(names, operation.desired),
      .ordering = operation.ordering,
      .failure_ordering = operation.failure_ordering,
      .rmw_opcode = operation.rmw_opcode,
      .result_mode = operation.result_mode,
      .address_space = operation.address_space,
      .missing_required_facts = {},
  };

  if (carrier.operation_kind == bir::AtomicOperationKind::None) {
    append_atomic_missing_fact(function_operations, carrier, "missing_atomic_operation_kind");
  }
  if (carrier.block_label == kInvalidBlockLabel) {
    append_atomic_missing_fact(function_operations, carrier, "missing_atomic_block_label");
  }
  if (carrier.operation_kind != bir::AtomicOperationKind::Fence &&
      carrier.value_type == bir::TypeKind::Void) {
    append_atomic_missing_fact(function_operations, carrier, "missing_atomic_value_type");
  }
  if (carrier.operation_kind != bir::AtomicOperationKind::Fence && carrier.width_bytes == 0) {
    append_atomic_missing_fact(function_operations, carrier, "missing_atomic_width");
  }
  if (carrier.ordering == bir::AtomicOrdering::None) {
    append_atomic_missing_fact(function_operations, carrier, "missing_atomic_ordering");
  }
  switch (carrier.operation_kind) {
    case bir::AtomicOperationKind::Load:
      if (!carrier.result.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "load_requires_result");
      }
      if (!carrier.pointer.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "load_requires_pointer");
      }
      if (carrier.result_mode == bir::AtomicResultMode::None) {
        append_atomic_missing_fact(function_operations, carrier, "load_requires_result_mode");
      }
      break;
    case bir::AtomicOperationKind::Store:
      if (!carrier.pointer.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "store_requires_pointer");
      }
      if (!carrier.value.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "store_requires_value");
      }
      break;
    case bir::AtomicOperationKind::Fence:
      break;
    case bir::AtomicOperationKind::Rmw:
      if (!carrier.result.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "rmw_requires_result");
      }
      if (!carrier.pointer.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "rmw_requires_pointer");
      }
      if (!carrier.value.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "rmw_requires_value");
      }
      if (carrier.rmw_opcode == bir::AtomicRmwOpcode::None) {
        append_atomic_missing_fact(function_operations, carrier, "rmw_requires_opcode");
      }
      if (carrier.result_mode == bir::AtomicResultMode::None) {
        append_atomic_missing_fact(function_operations, carrier, "rmw_requires_result_mode");
      }
      break;
    case bir::AtomicOperationKind::CompareExchange:
      if (!carrier.result.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "compare_exchange_requires_result");
      }
      if (!carrier.pointer.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "compare_exchange_requires_pointer");
      }
      if (!carrier.expected.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "compare_exchange_requires_expected");
      }
      if (!carrier.desired.has_value()) {
        append_atomic_missing_fact(function_operations, carrier, "compare_exchange_requires_desired");
      }
      if (carrier.failure_ordering == bir::AtomicOrdering::None) {
        append_atomic_missing_fact(
            function_operations, carrier, "compare_exchange_requires_failure_ordering");
      }
      if (carrier.result_mode == bir::AtomicResultMode::None) {
        append_atomic_missing_fact(
            function_operations, carrier, "compare_exchange_requires_result_mode");
      }
      break;
    case bir::AtomicOperationKind::None:
      break;
  }
  if (carrier.missing_required_facts.empty()) {
    carrier.carrier_kind = PreparedAtomicOperationCarrierKind::Complete;
  }
  return carrier;
}

}  // namespace

void populate_atomic_operations(PreparedBirModule& prepared) {
  prepared.atomic_operations.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.atomic_operations.empty()) {
      continue;
    }
    const FunctionNameId function_name =
        prepared.names.function_names.find(function.name);
    if (function_name == kInvalidFunctionName) {
      continue;
    }
    PreparedAtomicOperationFunction function_operations{
        .function_name = function_name,
        .operations = {},
        .missing_required_facts = {},
    };
    for (const auto& operation : function.atomic_operations) {
      function_operations.operations.push_back(
          build_atomic_operation_carrier(
              prepared.names, prepared.module.names, function_operations, operation));
    }
    prepared.atomic_operations.functions.push_back(std::move(function_operations));
  }
}

}  // namespace c4c::backend::prepare
