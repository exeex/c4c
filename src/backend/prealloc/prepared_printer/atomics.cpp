#include "private.hpp"

#include <sstream>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {

namespace atomic_printer {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

std::string_view maybe_value_name(const PreparedNameTables& names, ValueNameId id) {
  if (id == kInvalidValueName) {
    return "<none>";
  }
  return prepared_value_name(names, id);
}

std::string type_kind_name(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::Void:
      return "void";
    case c4c::backend::bir::TypeKind::I1:
      return "i1";
    case c4c::backend::bir::TypeKind::I8:
      return "i8";
    case c4c::backend::bir::TypeKind::I16:
      return "i16";
    case c4c::backend::bir::TypeKind::I32:
      return "i32";
    case c4c::backend::bir::TypeKind::I64:
      return "i64";
    case c4c::backend::bir::TypeKind::I128:
      return "i128";
    case c4c::backend::bir::TypeKind::Ptr:
      return "ptr";
    case c4c::backend::bir::TypeKind::F32:
      return "f32";
    case c4c::backend::bir::TypeKind::F64:
      return "f64";
    case c4c::backend::bir::TypeKind::F128:
      return "f128";
  }
  return "unknown";
}

std::string_view address_space_name(bir::AddressSpace address_space) {
  switch (address_space) {
    case bir::AddressSpace::Default:
      return "default";
    case bir::AddressSpace::Fs:
      return "fs";
    case bir::AddressSpace::Gs:
      return "gs";
    case bir::AddressSpace::Tls:
      return "tls";
  }
  return "unknown";
}

}  // namespace atomic_printer

void append_atomic_operations(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace atomic_printer;

  out << "--- prepared-atomic-operations ---\n";
  for (const auto& function_operations : module.atomic_operations.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_operations.function_name)
        << "\n";
    for (const auto& operation : function_operations.operations) {
      if (operation.carrier_kind != PreparedAtomicOperationCarrierKind::Complete) {
        continue;
      }
      out << "  atomic_operation kind="
          << bir::atomic_operation_kind_name(operation.operation_kind)
          << " block=" << maybe_block_label(module.names, operation.block_label)
          << " inst_index=" << operation.inst_index
          << " type=" << type_kind_name(operation.value_type)
          << " width=" << operation.width_bytes
          << " ordering=" << bir::atomic_ordering_name(operation.ordering)
          << " failure_ordering="
          << bir::atomic_ordering_name(operation.failure_ordering)
          << " result_mode="
          << bir::atomic_result_mode_name(operation.result_mode)
          << " rmw_opcode=" << bir::atomic_rmw_opcode_name(operation.rmw_opcode)
          << " address_space=" << address_space_name(operation.address_space);
      if (operation.result_value_name.has_value()) {
        out << " result=" << maybe_value_name(module.names, *operation.result_value_name);
      }
      if (operation.pointer_value_name.has_value()) {
        out << " pointer=" << maybe_value_name(module.names, *operation.pointer_value_name);
      }
      if (operation.value_name.has_value()) {
        out << " value=" << maybe_value_name(module.names, *operation.value_name);
      }
      if (operation.expected_value_name.has_value()) {
        out << " expected=" << maybe_value_name(module.names, *operation.expected_value_name);
      }
      if (operation.desired_value_name.has_value()) {
        out << " desired=" << maybe_value_name(module.names, *operation.desired_value_name);
      }
      out << "\n";
    }
    for (const auto& fact : function_operations.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
