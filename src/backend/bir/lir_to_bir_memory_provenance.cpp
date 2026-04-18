#include "lir_to_bir.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

using DynamicGlobalPointerArrayAccess = BirFunctionLowerer::DynamicGlobalPointerArrayAccess;
using GlobalAddress = BirFunctionLowerer::GlobalAddress;
using GlobalPointerSlotKey = BirFunctionLowerer::GlobalPointerSlotKey;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::type_size_bytes;

std::optional<std::vector<bir::Value>> BirFunctionLowerer::collect_local_pointer_values(
    const std::vector<std::string>& element_slots,
    const LocalPointerValueAliasMap& local_pointer_value_aliases) {
  if (element_slots.empty()) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(element_slots.size());
  for (const auto& slot_name : element_slots) {
    const auto alias_it = local_pointer_value_aliases.find(slot_name);
    if (alias_it == local_pointer_value_aliases.end() || alias_it->second.type != bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    element_values.push_back(alias_it->second);
  }
  return element_values;
}

std::optional<std::vector<bir::Value>> BirFunctionLowerer::collect_global_array_pointer_values(
    const DynamicGlobalPointerArrayAccess& access,
    const GlobalTypes& global_types) {
  const auto global_it = global_types.find(access.global_name);
  if (global_it == global_types.end() || access.element_count == 0) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(access.element_count);
  const auto element_stride =
      access.element_stride_bytes == 0 ? type_size_bytes(bir::TypeKind::Ptr)
                                       : access.element_stride_bytes;
  for (std::size_t index = 0; index < access.element_count; ++index) {
    const auto offset = access.byte_offset + index * element_stride;
    const auto init_it = global_it->second.pointer_initializer_offsets.find(offset);
    if (init_it == global_it->second.pointer_initializer_offsets.end() ||
        init_it->second.value_type != bir::TypeKind::Ptr || init_it->second.byte_offset != 0) {
      return std::nullopt;
    }
    element_values.push_back(
        bir::Value::named(bir::TypeKind::Ptr, "@" + init_it->second.global_name));
  }
  return element_values;
}

void BirFunctionLowerer::record_pointer_global_object_alias(
    std::string_view result_name,
    const GlobalInfo& global_info,
    const GlobalTypes& global_types,
    GlobalObjectPointerMap& global_object_pointer_slots) {
  if (global_info.initializer_symbol_name.empty() ||
      global_info.initializer_offset_type != bir::TypeKind::Void ||
      global_info.initializer_byte_offset != 0) {
    return;
  }

  const auto pointee_it = global_types.find(global_info.initializer_symbol_name);
  if (pointee_it == global_types.end() || !pointee_it->second.supports_direct_value ||
      pointee_it->second.value_type != bir::TypeKind::Ptr) {
    return;
  }

  global_object_pointer_slots[std::string(result_name)] = GlobalAddress{
      .global_name = global_info.initializer_symbol_name,
      .value_type = bir::TypeKind::Ptr,
      .byte_offset = 0,
  };
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_pointer_store_address(
    const c4c::codegen::lir::LirOperand& operand,
    const GlobalPointerMap& global_pointer_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = operand.str().substr(1);
    const auto global_it = global_types.find(global_name);
    if (global_it == global_types.end()) {
      if (!is_known_function_symbol(global_name, function_symbols)) {
        return std::nullopt;
      }
      return GlobalAddress{
          .global_name = global_name,
          .value_type = bir::TypeKind::Ptr,
          .byte_offset = 0,
      };
    }
    if (!global_it->second.supports_linear_addressing) {
      return std::nullopt;
    }
    return GlobalAddress{
        .global_name = global_name,
        .value_type = global_it->second.value_type,
        .byte_offset = 0,
    };
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto global_ptr_it = global_pointer_slots.find(operand.str());
  if (global_ptr_it == global_pointer_slots.end()) {
    return std::nullopt;
  }
  return global_ptr_it->second;
}

std::optional<bir::Value> BirFunctionLowerer::resolve_local_aggregate_pointer_value_alias(
    const c4c::codegen::lir::LirOperand& operand,
    const ValueMap& value_aliases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    if (local_aggregate_slots.find(operand.str()) != local_aggregate_slots.end()) {
      return bir::Value::named(bir::TypeKind::Ptr, operand.str());
    }
    return BirFunctionLowerer::lower_value(operand, bir::TypeKind::Ptr, value_aliases);
  }
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Global) {
    return std::nullopt;
  }

  const std::string symbol_name = operand.str().substr(1);
  if (!is_known_function_symbol(symbol_name, function_symbols)) {
    return std::nullopt;
  }
  return bir::Value::named(bir::TypeKind::Ptr, "@" + symbol_name);
}

std::optional<bir::Value> BirFunctionLowerer::lower_call_pointer_arg_value(
    const c4c::codegen::lir::LirOperand& operand,
    const ValueMap& value_aliases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    if (local_aggregate_slots.find(operand.str()) != local_aggregate_slots.end()) {
      return bir::Value::named(bir::TypeKind::Ptr, operand.str());
    }
    return BirFunctionLowerer::lower_value(operand, bir::TypeKind::Ptr, value_aliases);
  }
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Global) {
    return std::nullopt;
  }

  const std::string symbol_name = operand.str().substr(1);
  if (!is_known_function_symbol(symbol_name, function_symbols) &&
      global_types.find(symbol_name) == global_types.end()) {
    return std::nullopt;
  }
  return bir::Value::named(bir::TypeKind::Ptr, "@" + symbol_name);
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_honest_pointer_base(
    const GlobalAddress& address,
    const GlobalTypes& global_types) {
  if (address.value_type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  const auto global_it = global_types.find(address.global_name);
  if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
      global_it->second.value_type != bir::TypeKind::Ptr ||
      !global_it->second.known_global_address.has_value()) {
    return std::nullopt;
  }

  auto resolved = *global_it->second.known_global_address;
  resolved.byte_offset += address.byte_offset;

  const auto base_it = global_types.find(resolved.global_name);
  if (base_it == global_types.end() || !base_it->second.supports_linear_addressing ||
      resolved.byte_offset >= base_it->second.storage_size_bytes) {
    return std::nullopt;
  }

  return resolved;
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_honest_addressed_global_access(
    const GlobalAddress& address,
    bir::TypeKind accessed_type,
    const GlobalTypes& global_types) {
  if (accessed_type == bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  auto resolved = resolve_honest_pointer_base(address, global_types);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  resolved->value_type = accessed_type;
  return resolved;
}

GlobalPointerSlotKey BirFunctionLowerer::make_global_pointer_slot_key(const GlobalAddress& address) {
  return GlobalPointerSlotKey{
      .global_name = address.global_name,
      .byte_offset = address.byte_offset,
  };
}

bool BirFunctionLowerer::ensure_local_scratch_slot(std::string_view slot_name,
                                                   bir::TypeKind type,
                                                   std::size_t align_bytes) {
  const std::string owned_slot_name(slot_name);
  const auto slot_it = local_slot_types_.find(owned_slot_name);
  if (slot_it != local_slot_types_.end()) {
    return slot_it->second == type;
  }

  const auto slot_size = type_size_bytes(type);
  if (slot_size == 0) {
    return false;
  }

  local_slot_types_.emplace(owned_slot_name, type);
  lowered_function_.local_slots.push_back(bir::LocalSlot{
      .name = owned_slot_name,
      .type = type,
      .size_bytes = slot_size,
      .align_bytes = align_bytes == 0 ? slot_size : align_bytes,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  return true;
}

}  // namespace c4c::backend
