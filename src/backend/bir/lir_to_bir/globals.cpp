#include "lowering.hpp"

namespace c4c::backend::lir_to_bir_detail {

namespace {

std::optional<bir::TypeKind> lower_scalar_global_type(std::string_view text) {
  if (const auto lowered = lower_integer_type(text); lowered.has_value()) {
    return lowered;
  }
  if (text == "float" || text == "f32") {
    return bir::TypeKind::F32;
  }
  if (text == "double" || text == "f64") {
    return bir::TypeKind::F64;
  }
  return std::nullopt;
}

}  // namespace

std::optional<IntegerArrayType> parse_integer_array_type(std::string_view text) {
  IntegerArrayType lowered;
  std::string_view remainder = text;
  while (true) {
    if (const auto scalar_type = lower_integer_type(remainder); scalar_type.has_value()) {
      if (lowered.extents.empty()) {
        return std::nullopt;
      }
      lowered.element_type = *scalar_type;
      return lowered;
    }

    if (remainder.size() < 6 || remainder.front() != '[' || remainder.back() != ']') {
      return std::nullopt;
    }

    const auto x_pos = remainder.find(" x ");
    if (x_pos == std::string_view::npos || x_pos <= 1) {
      return std::nullopt;
    }

    const auto count = parse_i64(remainder.substr(1, x_pos - 1));
    if (!count.has_value() || *count <= 0) {
      return std::nullopt;
    }

    lowered.extents.push_back(static_cast<std::size_t>(*count));
    remainder = remainder.substr(x_pos + 3, remainder.size() - x_pos - 4);
  }
}

namespace {

std::optional<bir::Global> lower_scalar_global(const c4c::codegen::lir::LirGlobal& global,
                                               const TypeDeclMap& type_decls) {
  const auto lowered_type = lower_scalar_global_type(global.llvm_type);
  if (!lowered_type.has_value()) {
    return std::nullopt;
  }

  bir::Global lowered;
  lowered.name = global.name;
  lowered.type = *lowered_type;
  lowered.is_extern = global.is_extern_decl;
  lowered.is_constant = global.is_const;
  lowered.align_bytes = global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes) : 0;
  if (!global.is_extern_decl) {
    if (*lowered_type == bir::TypeKind::Ptr) {
      const auto trimmed_init = strip_typed_initializer_prefix(global.init_text, global.llvm_type);
      if (const auto initializer = lower_global_initializer(trimmed_init, *lowered_type);
          initializer.has_value()) {
        lowered.initializer = *initializer;
      } else {
        const auto initializer_address =
            parse_global_address_initializer(trimmed_init, type_decls);
        if (!initializer_address.has_value()) {
          return std::nullopt;
        }
        lowered.initializer_symbol_name = initializer_address->global_name;
      }
    } else {
      const auto trimmed_init = strip_typed_initializer_prefix(global.init_text, global.llvm_type);
      const auto initializer = lower_global_initializer(trimmed_init, *lowered_type);
      if (!initializer.has_value()) {
        return std::nullopt;
      }
      lowered.initializer = *initializer;
    }
  }
  return lowered;
}

}  // namespace

bool is_known_function_symbol(std::string_view symbol_name,
                              const FunctionSymbolSet& function_symbols) {
  return function_symbols.find(std::string(symbol_name)) != function_symbols.end();
}

std::optional<GlobalAddress> resolve_known_global_address(std::string_view global_name,
                                                          GlobalTypes& global_types,
                                                          const FunctionSymbolSet& function_symbols,
                                                          std::unordered_set<std::string>* active) {
  const auto it = global_types.find(std::string(global_name));
  if (it == global_types.end()) {
    return std::nullopt;
  }

  auto& info = it->second;
  if (info.known_global_address.has_value()) {
    return info.known_global_address;
  }
  if (info.initializer_symbol_name.empty()) {
    return std::nullopt;
  }
  if (is_known_function_symbol(info.initializer_symbol_name, function_symbols)) {
    if (info.initializer_offset_type != bir::TypeKind::Void || info.initializer_byte_offset != 0) {
      return std::nullopt;
    }
    info.known_global_address = GlobalAddress{
        .global_name = info.initializer_symbol_name,
        .value_type = bir::TypeKind::Ptr,
        .byte_offset = 0,
    };
    return info.known_global_address;
  }

  const std::string active_name(global_name);
  if (!active->insert(active_name).second) {
    return std::nullopt;
  }

  const auto erase_active = [&]() { active->erase(active_name); };
  const auto pointee_it = global_types.find(info.initializer_symbol_name);
  if (pointee_it == global_types.end()) {
    erase_active();
    return std::nullopt;
  }

  GlobalAddress resolved;
  if (pointee_it->second.supports_linear_addressing) {
    resolved = GlobalAddress{
        .global_name = info.initializer_symbol_name,
        .value_type = info.initializer_offset_type != bir::TypeKind::Void
                          ? info.initializer_offset_type
                          : pointee_it->second.value_type,
        .byte_offset = info.initializer_byte_offset,
    };
  } else {
    const auto pointee_address =
        resolve_known_global_address(
            info.initializer_symbol_name, global_types, function_symbols, active);
    if (!pointee_address.has_value()) {
      erase_active();
      return std::nullopt;
    }
    resolved = *pointee_address;
    auto nested_offset = info.initializer_byte_offset;
    if (info.initializer_offset_type == bir::TypeKind::I8 &&
        pointee_it->second.value_type == bir::TypeKind::Ptr &&
        resolved.value_type != bir::TypeKind::I8) {
      const auto pointee_stride = type_size_bytes(resolved.value_type);
      if (pointee_stride == 0) {
        erase_active();
        return std::nullopt;
      }
      nested_offset *= pointee_stride;
    }
    resolved.byte_offset += nested_offset;
  }

  erase_active();

  const auto base_it = global_types.find(resolved.global_name);
  if (base_it == global_types.end() || !base_it->second.supports_linear_addressing) {
    return std::nullopt;
  }
  if (resolved.byte_offset >= base_it->second.storage_size_bytes) {
    return std::nullopt;
  }

  info.known_global_address = resolved;
  return info.known_global_address;
}

bool resolve_pointer_initializer_offsets(GlobalTypes& global_types,
                                         const FunctionSymbolSet& function_symbols) {
  std::unordered_set<std::string> resolving_global_addresses;
  for (auto& [global_name, info] : global_types) {
    (void)global_name;
    for (auto& [byte_offset, address] : info.pointer_initializer_offsets) {
      if (address.value_type != bir::TypeKind::Void) {
        continue;
      }
      if (byte_offset >= info.storage_size_bytes) {
        return false;
      }

      const auto target_it = global_types.find(address.global_name);
      if (target_it == global_types.end()) {
        if (!is_known_function_symbol(address.global_name, function_symbols) ||
            address.byte_offset != 0) {
          return false;
        }
        address.value_type = bir::TypeKind::Ptr;
        continue;
      }

      if (target_it->second.supports_linear_addressing) {
        if (address.value_type == bir::TypeKind::Void) {
          address.value_type = target_it->second.value_type;
        }
        if (address.byte_offset >= target_it->second.storage_size_bytes) {
          return false;
        }
        continue;
      }

      if (target_it->second.supports_direct_value &&
          target_it->second.value_type == bir::TypeKind::Ptr &&
          address.byte_offset == 0) {
        address.value_type = bir::TypeKind::Ptr;
        continue;
      }

      const auto resolved_address =
          resolve_known_global_address(
              address.global_name, global_types, function_symbols, &resolving_global_addresses);
      if (!resolved_address.has_value()) {
        return false;
      }
      address = *resolved_address;
    }
  }
  return true;
}

std::optional<bir::Global> lower_minimal_global(const c4c::codegen::lir::LirGlobal& global,
                                                const TypeDeclMap& type_decls,
                                                GlobalInfo* info) {
  if (auto lowered = lower_scalar_global(global, type_decls); lowered.has_value()) {
    info->value_type = lowered->type;
    info->element_size_bytes = type_size_bytes(lowered->type);
    info->element_count = 1;
    info->storage_size_bytes = info->element_size_bytes;
    info->supports_direct_value = true;
    info->supports_linear_addressing = true;
    if (lowered->initializer_symbol_name.has_value()) {
      const auto initializer_address = parse_global_address_initializer(
          strip_typed_initializer_prefix(global.init_text, global.llvm_type),
          type_decls);
      if (!initializer_address.has_value()) {
        return std::nullopt;
      }
      info->initializer_symbol_name = initializer_address->global_name;
      info->initializer_offset_type = initializer_address->value_type;
      info->initializer_byte_offset = initializer_address->byte_offset;
      info->supports_linear_addressing = false;
    }
    if (lowered->size_bytes == 0) {
      lowered->size_bytes = info->element_size_bytes;
    }
    return lowered;
  }

  if (const auto integer_array = parse_integer_array_type(global.llvm_type);
      integer_array.has_value() && integer_array->element_type != bir::TypeKind::Ptr) {
    const auto element_size_bytes = type_size_bytes(integer_array->element_type);
    if (element_size_bytes == 0) {
      return std::nullopt;
    }

    std::size_t total_elements = 1;
    for (const auto extent : integer_array->extents) {
      total_elements *= extent;
    }

    bir::Global lowered;
    lowered.name = global.name;
    lowered.type = integer_array->element_type;
    lowered.is_extern = global.is_extern_decl;
    lowered.is_constant = global.is_const;
    lowered.size_bytes = total_elements * element_size_bytes;
    lowered.align_bytes =
        global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes) : 0;
    if (!global.is_extern_decl) {
      const auto initializer_elements =
          lower_integer_array_initializer(global.init_text, global.llvm_type);
      if (!initializer_elements.has_value()) {
        return std::nullopt;
      }
      lowered.initializer_elements = *initializer_elements;
    }

    info->value_type = integer_array->element_type;
    info->element_size_bytes = element_size_bytes;
    info->element_count = total_elements;
    info->storage_size_bytes = lowered.size_bytes;
    info->supports_direct_value = false;
    info->supports_linear_addressing = true;
    info->type_text = global.llvm_type;
    return lowered;
  }

  const auto layout = compute_aggregate_type_layout(global.llvm_type, type_decls);
  if ((layout.kind != AggregateTypeLayout::Kind::Struct &&
       layout.kind != AggregateTypeLayout::Kind::Array) ||
      layout.size_bytes == 0) {
    return std::nullopt;
  }

  bir::Global aggregate;
  aggregate.name = global.name;
  aggregate.type = bir::TypeKind::I8;
  aggregate.is_extern = global.is_extern_decl;
  aggregate.is_constant = global.is_const;
  aggregate.size_bytes = layout.size_bytes;
  aggregate.align_bytes = global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes)
                                                 : layout.align_bytes;
  if (!global.is_extern_decl) {
    std::unordered_map<std::size_t, GlobalAddress> pointer_offsets;
    const auto initializer_elements =
        lower_aggregate_initializer(global.init_text, global.llvm_type, type_decls, &pointer_offsets);
    if (!initializer_elements.has_value()) {
      return std::nullopt;
    }
    aggregate.initializer_elements = *initializer_elements;
    info->pointer_initializer_offsets = std::move(pointer_offsets);
  }

  info->value_type = bir::TypeKind::I8;
  info->element_size_bytes = 1;
  info->element_count = layout.size_bytes;
  info->storage_size_bytes = layout.size_bytes;
  info->supports_direct_value = false;
  info->supports_linear_addressing = true;
  info->type_text = global.llvm_type;
  return aggregate;
}

std::optional<bir::Global> lower_string_constant_global(
    const c4c::codegen::lir::LirStringConst& string_constant,
    GlobalInfo* info) {
  if (string_constant.pool_name.empty() || string_constant.byte_length <= 0) {
    return std::nullopt;
  }

  bir::Global lowered;
  lowered.name = string_constant.pool_name.front() == '@'
                     ? string_constant.pool_name.substr(1)
                     : string_constant.pool_name;
  lowered.type = bir::TypeKind::I8;
  lowered.is_extern = true;
  lowered.is_constant = true;
  lowered.size_bytes = static_cast<std::size_t>(string_constant.byte_length);
  lowered.align_bytes = 1;

  info->value_type = bir::TypeKind::I8;
  info->element_size_bytes = 1;
  info->element_count = static_cast<std::size_t>(string_constant.byte_length);
  info->storage_size_bytes = static_cast<std::size_t>(string_constant.byte_length);
  info->supports_direct_value = false;
  info->supports_linear_addressing = true;
  info->type_text = "[" + std::to_string(string_constant.byte_length) + " x i8]";
  return lowered;
}

}  // namespace c4c::backend::lir_to_bir_detail
