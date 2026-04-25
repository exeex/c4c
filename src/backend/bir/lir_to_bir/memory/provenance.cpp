#include "../lowering.hpp"
#include "memory_helpers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

using DynamicGlobalPointerArrayAccess = BirFunctionLowerer::DynamicGlobalPointerArrayAccess;
using GlobalAddress = BirFunctionLowerer::GlobalAddress;
using GlobalPointerSlotKey = BirFunctionLowerer::GlobalPointerSlotKey;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::type_size_bytes;

static bool can_address_scalar_subobject(std::int64_t byte_offset,
                                         bir::TypeKind stored_type,
                                         std::string_view type_text,
                                         bir::TypeKind access_type,
                                         const BirFunctionLowerer::TypeDeclMap& type_decls,
                                         bool allow_opaque_ptr_base) {
  if (byte_offset < 0) {
    return false;
  }
  const auto access_size = type_size_bytes(access_type);
  if (access_size == 0) {
    return false;
  }
  if (stored_type == access_type) {
    return true;
  }
  if (stored_type != bir::TypeKind::Void) {
    const auto stored_size = type_size_bytes(stored_type);
    if (stored_size == 0 ||
        static_cast<std::size_t>(byte_offset) + access_size > stored_size) {
      return false;
    }
    // Preserve byte-wise inspection/update of scalar object representations
    // after pointer reinterpretation through unsigned char* style views.
    return access_type == bir::TypeKind::I8;
  }
  if (allow_opaque_ptr_base && byte_offset == 0 && (type_text.empty() || type_text == "ptr")) {
    return true;
  }

  const auto scalar_facts = resolve_scalar_layout_facts_at_byte_offset(
      type_text, static_cast<std::size_t>(byte_offset), type_decls);
  if (!scalar_facts.has_value() || scalar_facts->object_size_bytes == 0 ||
      scalar_facts->target_byte_offset != static_cast<std::size_t>(byte_offset)) {
    return false;
  }
  return access_size <= scalar_facts->remaining_object_bytes;
}

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

static std::optional<BirFunctionLowerer::PointerAddress> make_runtime_global_pointer_address(
    std::string_view result_name,
    const lir_to_bir_detail::GlobalInfo& global_info) {
  if (global_info.runtime_element_count == 0 || global_info.runtime_element_stride_bytes == 0) {
    return std::nullopt;
  }
  return BirFunctionLowerer::PointerAddress{
      .base_value = bir::Value::named(bir::TypeKind::Ptr, std::string(result_name)),
      .dynamic_element_count = global_info.runtime_element_count,
      .dynamic_element_stride_bytes = global_info.runtime_element_stride_bytes,
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

static std::optional<BirFunctionLowerer::PointerAddress> resolve_pointer_store_value_address(
    const c4c::codegen::lir::LirOperand& operand,
    const BirFunctionLowerer::PointerAddressMap& pointer_value_addresses) {
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto addressed_ptr_it = pointer_value_addresses.find(operand.str());
  if (addressed_ptr_it == pointer_value_addresses.end()) {
    return std::nullopt;
  }
  return addressed_ptr_it->second;
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

static std::optional<GlobalAddress> resolve_linear_addressed_global_scalar_access(
    const GlobalAddress& address,
    bir::TypeKind accessed_type,
    const BirFunctionLowerer::GlobalTypes& global_types,
    const BirFunctionLowerer::TypeDeclMap& type_decls) {
  const auto global_it = global_types.find(address.global_name);
  if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
    return std::nullopt;
  }

  if (!can_address_scalar_subobject(static_cast<std::int64_t>(address.byte_offset),
                                    bir::TypeKind::Void,
                                    global_it->second.type_text,
                                    accessed_type,
                                    type_decls,
                                    false)) {
    return std::nullopt;
  }

  return GlobalAddress{
      .global_name = address.global_name,
      .value_type = accessed_type,
      .byte_offset = address.byte_offset,
  };
}

GlobalPointerSlotKey BirFunctionLowerer::make_global_pointer_slot_key(const GlobalAddress& address) {
  return GlobalPointerSlotKey{
      .global_name = address.global_name,
      .byte_offset = address.byte_offset,
  };
}

std::optional<bool> BirFunctionLowerer::try_lower_global_provenance_load(
    const c4c::codegen::lir::LirLoadOp& load,
    bir::TypeKind value_type,
    const GlobalTypes& global_types,
    const TypeDeclMap& type_decls,
    const GlobalAddressSlots& global_address_slots,
    const AddressedGlobalPointerSlots& addressed_global_pointer_slots,
    const GlobalPointerValueSlots& global_pointer_value_slots,
    const AddressedGlobalPointerValueSlots& addressed_global_pointer_value_slots,
    GlobalPointerMap* global_pointer_slots,
    GlobalObjectPointerMap* global_object_pointer_slots,
    PointerAddressMap* pointer_value_addresses,
    std::vector<bir::Inst>* lowered_insts) {
  if (load.ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = load.ptr.str().substr(1);
    const auto global_it = global_types.find(global_name);
    if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
        global_it->second.value_type != value_type) {
      return false;
    }
    if (value_type == bir::TypeKind::Ptr) {
      if (const auto runtime_it = global_address_slots.find(global_name);
          runtime_it != global_address_slots.end()) {
        if (runtime_it->second.has_value()) {
          (*global_pointer_slots)[load.result.str()] = *runtime_it->second;
        }
      } else if (const auto runtime_global =
                     make_runtime_global_pointer_address(load.result.str(), global_it->second);
                 runtime_global.has_value()) {
        (*pointer_value_addresses)[load.result.str()] = *runtime_global;
      } else if (const auto runtime_ptr_it = global_pointer_value_slots.find(global_name);
                 runtime_ptr_it != global_pointer_value_slots.end()) {
        if (runtime_ptr_it->second.has_value()) {
          auto runtime_pointer = *runtime_ptr_it->second;
          runtime_pointer.base_value = bir::Value::named(bir::TypeKind::Ptr, load.result.str());
          (*pointer_value_addresses)[load.result.str()] = std::move(runtime_pointer);
        }
      } else if (global_it->second.known_global_address.has_value()) {
        (*global_pointer_slots)[load.result.str()] = *global_it->second.known_global_address;
        record_pointer_global_object_alias(
            load.result.str(), global_it->second, global_types, *global_object_pointer_slots);
      }
    }
    lowered_insts->push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(value_type, load.result.str()),
        .global_name = global_name,
    });
    return true;
  }

  if (value_type == bir::TypeKind::Ptr) {
    if (const auto global_object_it = global_object_pointer_slots->find(load.ptr.str());
        global_object_it != global_object_pointer_slots->end()) {
      const auto global_it = global_types.find(global_object_it->second.global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != bir::TypeKind::Ptr) {
        return false;
      }
      if (const auto runtime_it = global_address_slots.find(global_object_it->second.global_name);
          runtime_it != global_address_slots.end()) {
        if (runtime_it->second.has_value()) {
          (*global_pointer_slots)[load.result.str()] = *runtime_it->second;
        }
      } else if (const auto runtime_global =
                     make_runtime_global_pointer_address(load.result.str(), global_it->second);
                 runtime_global.has_value()) {
        (*pointer_value_addresses)[load.result.str()] = *runtime_global;
      } else if (const auto runtime_ptr_it =
                     global_pointer_value_slots.find(global_object_it->second.global_name);
                 runtime_ptr_it != global_pointer_value_slots.end()) {
        if (runtime_ptr_it->second.has_value()) {
          auto runtime_pointer = *runtime_ptr_it->second;
          runtime_pointer.base_value = bir::Value::named(bir::TypeKind::Ptr, load.result.str());
          (*pointer_value_addresses)[load.result.str()] = std::move(runtime_pointer);
        }
      } else if (global_it->second.known_global_address.has_value()) {
        (*global_pointer_slots)[load.result.str()] = *global_it->second.known_global_address;
        record_pointer_global_object_alias(
            load.result.str(), global_it->second, global_types, *global_object_pointer_slots);
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(value_type, load.result.str()),
          .global_name = global_object_it->second.global_name,
          .byte_offset = global_object_it->second.byte_offset,
      });
      return true;
    }
  }

  const auto global_ptr_it = global_pointer_slots->find(load.ptr.str());
  if (global_ptr_it == global_pointer_slots->end()) {
    return std::nullopt;
  }

  if (value_type != bir::TypeKind::Ptr) {
    if (const auto honest_address =
            resolve_honest_addressed_global_access(global_ptr_it->second, value_type, global_types);
        honest_address.has_value()) {
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(value_type, load.result.str()),
          .global_name = honest_address->global_name,
          .byte_offset = honest_address->byte_offset,
      });
      return true;
    }
    if (const auto linear_address = resolve_linear_addressed_global_scalar_access(
            global_ptr_it->second, value_type, global_types, type_decls);
        linear_address.has_value()) {
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(value_type, load.result.str()),
          .global_name = linear_address->global_name,
          .byte_offset = linear_address->byte_offset,
      });
      return true;
    }
  }

  if (global_ptr_it->second.value_type != value_type) {
    if (global_ptr_it->second.value_type != bir::TypeKind::Ptr ||
        global_ptr_it->second.byte_offset != 0) {
      return false;
    }
    const auto global_it = global_types.find(global_ptr_it->second.global_name);
    if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
        global_it->second.value_type != bir::TypeKind::Ptr) {
      return false;
    }
    lowered_insts->push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(value_type, load.result.str()),
        .global_name = global_ptr_it->second.global_name,
    });
    return true;
  }

  if (value_type == bir::TypeKind::Ptr) {
    const auto addressed_key = make_global_pointer_slot_key(global_ptr_it->second);
    const auto addressed_it = addressed_global_pointer_slots.find(addressed_key);
    if (addressed_it != addressed_global_pointer_slots.end()) {
      if (addressed_it->second.has_value()) {
        (*global_pointer_slots)[load.result.str()] = *addressed_it->second;
      }
    } else if (const auto addressed_ptr_it =
                   addressed_global_pointer_value_slots.find(addressed_key);
               addressed_ptr_it != addressed_global_pointer_value_slots.end()) {
      if (addressed_ptr_it->second.has_value()) {
        auto runtime_pointer = *addressed_ptr_it->second;
        runtime_pointer.base_value = bir::Value::named(bir::TypeKind::Ptr, load.result.str());
        (*pointer_value_addresses)[load.result.str()] = std::move(runtime_pointer);
      }
    } else {
      const auto global_it = global_types.find(global_ptr_it->second.global_name);
      if (global_it == global_types.end()) {
        return false;
      }
      if (global_ptr_it->second.byte_offset == 0 && global_it->second.supports_direct_value &&
          global_it->second.value_type == bir::TypeKind::Ptr) {
        if (const auto runtime_it = global_address_slots.find(global_ptr_it->second.global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            (*global_pointer_slots)[load.result.str()] = *runtime_it->second;
          }
        } else if (const auto runtime_global =
                       make_runtime_global_pointer_address(load.result.str(), global_it->second);
                   runtime_global.has_value()) {
          (*pointer_value_addresses)[load.result.str()] = *runtime_global;
        } else if (const auto runtime_ptr_it =
                       global_pointer_value_slots.find(global_ptr_it->second.global_name);
                   runtime_ptr_it != global_pointer_value_slots.end()) {
          if (runtime_ptr_it->second.has_value()) {
            auto runtime_pointer = *runtime_ptr_it->second;
            runtime_pointer.base_value = bir::Value::named(bir::TypeKind::Ptr, load.result.str());
            (*pointer_value_addresses)[load.result.str()] = std::move(runtime_pointer);
          }
        } else if (global_it->second.known_global_address.has_value()) {
          (*global_pointer_slots)[load.result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load.result.str(), global_it->second, global_types, *global_object_pointer_slots);
        }
      }
      const auto pointer_init_it =
          global_it->second.pointer_initializer_offsets.find(global_ptr_it->second.byte_offset);
      if (pointer_init_it != global_it->second.pointer_initializer_offsets.end()) {
        (*global_pointer_slots)[load.result.str()] = pointer_init_it->second;
      }
    }
  }

  lowered_insts->push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(value_type, load.result.str()),
      .global_name = global_ptr_it->second.global_name,
      .byte_offset = global_ptr_it->second.byte_offset,
  });
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_global_provenance_store(
    const c4c::codegen::lir::LirStoreOp& store,
    bir::TypeKind value_type,
    const bir::Value& value,
    const TypeDeclMap& type_decls,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    const GlobalPointerMap& global_pointer_slots,
    const GlobalObjectPointerMap& global_object_pointer_slots,
    const PointerAddressMap& pointer_value_addresses,
    GlobalAddressSlots* global_address_slots,
    AddressedGlobalPointerSlots* addressed_global_pointer_slots,
    GlobalPointerValueSlots* global_pointer_value_slots,
    AddressedGlobalPointerValueSlots* addressed_global_pointer_value_slots,
    std::vector<bir::Inst>* lowered_insts) {
  if (store.ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = store.ptr.str().substr(1);
    const auto global_it = global_types.find(global_name);
    if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
        global_it->second.value_type != value_type) {
      return false;
    }
    if (value_type == bir::TypeKind::Ptr) {
      (*global_address_slots)[global_name] =
          resolve_pointer_store_address(store.val, global_pointer_slots, global_types, function_symbols);
      (*global_pointer_value_slots)[global_name] =
          resolve_pointer_store_value_address(store.val, pointer_value_addresses);
    }
    lowered_insts->push_back(bir::StoreGlobalInst{
        .global_name = global_name,
        .value = value,
    });
    return true;
  }

  if (value_type == bir::TypeKind::Ptr) {
    const auto global_object_it = global_object_pointer_slots.find(store.ptr.str());
    if (global_object_it != global_object_pointer_slots.end()) {
      const auto global_it = global_types.find(global_object_it->second.global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != value_type) {
        return false;
      }
      (*global_address_slots)[global_object_it->second.global_name] =
          resolve_pointer_store_address(store.val, global_pointer_slots, global_types, function_symbols);
      (*global_pointer_value_slots)[global_object_it->second.global_name] =
          resolve_pointer_store_value_address(store.val, pointer_value_addresses);
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_object_it->second.global_name,
          .value = value,
          .byte_offset = global_object_it->second.byte_offset,
      });
      return true;
    }
  }

  const auto global_ptr_it = global_pointer_slots.find(store.ptr.str());
  if (global_ptr_it == global_pointer_slots.end()) {
    return std::nullopt;
  }

  if (value_type != bir::TypeKind::Ptr) {
    if (const auto honest_address =
            resolve_honest_addressed_global_access(global_ptr_it->second, value_type, global_types);
        honest_address.has_value()) {
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = honest_address->global_name,
          .value = value,
          .byte_offset = honest_address->byte_offset,
      });
      return true;
    }
    if (const auto linear_address = resolve_linear_addressed_global_scalar_access(
            global_ptr_it->second, value_type, global_types, type_decls);
        linear_address.has_value()) {
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = linear_address->global_name,
          .value = value,
          .byte_offset = linear_address->byte_offset,
      });
      return true;
    }
  }

  if (global_ptr_it->second.value_type != value_type) {
    return false;
  }
  if (value_type == bir::TypeKind::Ptr) {
    (*addressed_global_pointer_slots)[make_global_pointer_slot_key(global_ptr_it->second)] =
        resolve_pointer_store_address(store.val, global_pointer_slots, global_types, function_symbols);
    (*addressed_global_pointer_value_slots)[make_global_pointer_slot_key(global_ptr_it->second)] =
        resolve_pointer_store_value_address(store.val, pointer_value_addresses);
  }
  lowered_insts->push_back(bir::StoreGlobalInst{
      .global_name = global_ptr_it->second.global_name,
      .value = value,
      .byte_offset = global_ptr_it->second.byte_offset,
  });
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_pointer_provenance_store(
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const bir::Value& value,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    const LocalSlotPointerValues& local_slot_pointer_values,
    const PointerAddressMap& pointer_value_addresses,
    std::vector<bir::Inst>* lowered_insts) {
  if (const auto addressed_store = try_lower_addressed_pointer_store(
          ptr_name, value_type, value, type_decls, pointer_value_addresses, lowered_insts);
      addressed_store.has_value()) {
    return addressed_store;
  }

  const auto local_slot_ptr_it = local_slot_pointer_values.find(std::string(ptr_name));
  if (local_slot_ptr_it == local_slot_pointer_values.end()) {
    return std::nullopt;
  }
  if (!can_address_scalar_subobject(local_slot_ptr_it->second.byte_offset,
                                    local_slot_ptr_it->second.value_type,
                                    local_slot_ptr_it->second.type_text,
                                    value_type,
                                    type_decls,
                                    false)) {
    return false;
  }
  return try_lower_local_slot_pointer_store(
      local_slot_ptr_it->second, value_type, value, local_slot_types, lowered_insts);
}

std::optional<bool> BirFunctionLowerer::try_lower_addressed_pointer_store(
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const bir::Value& value,
    const TypeDeclMap& type_decls,
    const PointerAddressMap& pointer_value_addresses,
    std::vector<bir::Inst>* lowered_insts) {
  const auto addressed_ptr_it = pointer_value_addresses.find(std::string(ptr_name));
  if (addressed_ptr_it == pointer_value_addresses.end()) {
    return std::nullopt;
  }

  if (!can_address_scalar_subobject(static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                                    addressed_ptr_it->second.value_type,
                                    addressed_ptr_it->second.type_text,
                                    value_type,
                                    type_decls,
                                    true)) {
    return false;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return false;
  }

  const std::string scratch_slot = std::string(ptr_name) + ".addr";
  if (!ensure_local_scratch_slot(scratch_slot, value_type, slot_size)) {
    return false;
  }

  lowered_insts->push_back(bir::StoreLocalInst{
      .slot_name = scratch_slot,
      .value = value,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_value = addressed_ptr_it->second.base_value,
              .byte_offset = static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
              .size_bytes = slot_size,
              .align_bytes = slot_size,
          },
  });
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_pointer_provenance_load(
    std::string_view result_name,
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
    const LocalAddressSlots& local_address_slots,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    ValueMap* value_aliases,
    LocalSlotPointerValues* local_slot_pointer_values,
    GlobalPointerMap* global_pointer_slots,
    const PointerAddressMap& pointer_value_addresses,
    std::vector<bir::Inst>* lowered_insts) {
  if (const auto addressed_load = try_lower_addressed_pointer_load(
          result_name,
          ptr_name,
          value_type,
          type_decls,
          pointer_value_addresses,
          &pointer_value_addresses_,
          lowered_insts);
      addressed_load.has_value()) {
    return addressed_load;
  }

  const auto local_slot_ptr_it = local_slot_pointer_values->find(std::string(ptr_name));
  if (local_slot_ptr_it == local_slot_pointer_values->end()) {
    return std::nullopt;
  }
  if (!can_address_scalar_subobject(local_slot_ptr_it->second.byte_offset,
                                    local_slot_ptr_it->second.value_type,
                                    local_slot_ptr_it->second.type_text,
                                    value_type,
                                    type_decls,
                                    false)) {
    return false;
  }
  return try_lower_local_slot_pointer_load(result_name,
                                           local_slot_ptr_it->second,
                                           value_type,
                                           local_slot_types,
                                           local_indirect_pointer_slots,
                                           local_address_slots,
                                           local_slot_address_slots,
                                           global_types,
                                           function_symbols,
                                           value_aliases,
                                           local_slot_pointer_values,
                                           global_pointer_slots,
                                           lowered_insts);
}

std::optional<bool> BirFunctionLowerer::try_lower_addressed_pointer_load(
    std::string_view result_name,
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const TypeDeclMap& type_decls,
    const PointerAddressMap& pointer_value_addresses,
    PointerAddressMap* loaded_pointer_value_addresses,
    std::vector<bir::Inst>* lowered_insts) {
  const auto addressed_ptr_it = pointer_value_addresses.find(std::string(ptr_name));
  if (addressed_ptr_it == pointer_value_addresses.end()) {
    return std::nullopt;
  }

  if (!can_address_scalar_subobject(static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                                    addressed_ptr_it->second.value_type,
                                    addressed_ptr_it->second.type_text,
                                    value_type,
                                    type_decls,
                                    true)) {
    return false;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return false;
  }

  const std::string scratch_slot = std::string(result_name) + ".addr";
  if (!ensure_local_scratch_slot(scratch_slot, value_type, slot_size)) {
    return false;
  }

  lowered_insts->push_back(bir::LoadLocalInst{
      .result = bir::Value::named(value_type, std::string(result_name)),
      .slot_name = scratch_slot,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_value = addressed_ptr_it->second.base_value,
              .byte_offset = static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
              .size_bytes = slot_size,
              .align_bytes = slot_size,
          },
  });
  if (value_type == bir::TypeKind::Ptr && loaded_pointer_value_addresses != nullptr) {
    // The loaded value is itself a runtime pointer and must remain directly
    // addressable for a follow-on `load` without forcing an intervening GEP.
    (*loaded_pointer_value_addresses)[std::string(result_name)] = PointerAddress{
        .base_value = bir::Value::named(bir::TypeKind::Ptr, std::string(result_name)),
        .value_type = bir::TypeKind::Void,
        .byte_offset = 0,
    };
  }
  return true;
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
