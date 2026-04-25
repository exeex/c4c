#include "lowering.hpp"

#include <array>
#include <charconv>
#include <cstring>

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

std::optional<std::string_view> peel_integer_array_layer(std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  return text.substr(x_pos + 3, text.size() - x_pos - 4);
}

std::optional<int> parse_hex_digit(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + (ch - 'A');
  }
  return std::nullopt;
}

bool append_x86_fp80_initializer_bytes(std::string_view text, std::vector<bir::Value>* out) {
  if (text.size() != 23 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X') ||
      (text[2] != 'K' && text[2] != 'k')) {
    return false;
  }

  std::array<std::uint8_t, 10> payload{};
  for (std::size_t index = 0; index < payload.size(); ++index) {
    const auto hi = parse_hex_digit(text[3 + index * 2]);
    const auto lo = parse_hex_digit(text[4 + index * 2]);
    if (!hi.has_value() || !lo.has_value()) {
      return false;
    }
    payload[index] = static_cast<std::uint8_t>((*hi << 4) | *lo);
  }

  for (auto it = payload.rbegin(); it != payload.rend(); ++it) {
    out->push_back(bir::Value::immediate_i8(static_cast<std::int8_t>(*it)));
  }
  for (std::size_t index = payload.size(); index < type_size_bytes(bir::TypeKind::F128); ++index) {
    out->push_back(bir::Value::immediate_i8(0));
  }
  return true;
}

std::optional<std::vector<bir::Value>> lower_llvm_byte_string_initializer(
    std::string_view init_text,
    std::string_view type_text) {
  const auto array_type = parse_integer_array_type(type_text);
  if (!array_type.has_value() || array_type->element_type != bir::TypeKind::I8 ||
      array_type->extents.size() != 1) {
    return std::nullopt;
  }

  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init.size() < 3 || trimmed_init[0] != 'c' || trimmed_init[1] != '"' ||
      trimmed_init.back() != '"') {
    return std::nullopt;
  }

  std::vector<bir::Value> lowered;
  lowered.reserve(array_type->extents.front());
  for (std::size_t index = 2; index + 1 < trimmed_init.size(); ++index) {
    unsigned char byte = static_cast<unsigned char>(trimmed_init[index]);
    if (trimmed_init[index] == '\\') {
      if (index + 2 >= trimmed_init.size() - 1) {
        return std::nullopt;
      }
      const auto hi = parse_hex_digit(trimmed_init[index + 1]);
      const auto lo = parse_hex_digit(trimmed_init[index + 2]);
      if (!hi.has_value() || !lo.has_value()) {
        return std::nullopt;
      }
      byte = static_cast<unsigned char>((*hi << 4) | *lo);
      index += 2;
    }
    lowered.push_back(bir::Value::immediate_i8(static_cast<std::int8_t>(byte)));
  }

  if (lowered.size() > array_type->extents.front()) {
    return std::nullopt;
  }
  while (lowered.size() < array_type->extents.front()) {
    lowered.push_back(bir::Value::immediate_i8(0));
  }
  return lowered;
}

}  // namespace

std::optional<bir::Value> lower_global_initializer(std::string_view text,
                                                   bir::TypeKind type) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  if (trimmed == "zeroinitializer") {
    switch (type) {
      case bir::TypeKind::I1:
        return bir::Value::immediate_i1(false);
      case bir::TypeKind::I8:
        return bir::Value::immediate_i8(0);
      case bir::TypeKind::I16:
        return bir::Value::immediate_i16(0);
      case bir::TypeKind::I32:
        return bir::Value::immediate_i32(0);
      case bir::TypeKind::I64:
        return bir::Value::immediate_i64(0);
      case bir::TypeKind::F32:
        return bir::Value::immediate_f32_bits(0u);
      case bir::TypeKind::F64:
        return bir::Value::immediate_f64_bits(0u);
      default:
        return std::nullopt;
    }
  }

  if (type == bir::TypeKind::Ptr && trimmed == "null") {
    return bir::Value{
        .kind = bir::Value::Kind::Immediate,
        .type = bir::TypeKind::Ptr,
        .immediate = 0,
        .immediate_bits = 0,
    };
  }

  if (type == bir::TypeKind::I1) {
    if (trimmed == "true") {
      return bir::Value::immediate_i1(true);
    }
    if (trimmed == "false") {
      return bir::Value::immediate_i1(false);
    }
  }

  if (type == bir::TypeKind::F32 || type == bir::TypeKind::F64) {
    if (trimmed.size() < 3 || trimmed[0] != '0' || (trimmed[1] != 'x' && trimmed[1] != 'X')) {
      return std::nullopt;
    }

    std::uint64_t bits = 0;
    const auto* begin = trimmed.data() + 2;
    const auto* end = trimmed.data() + trimmed.size();
    const auto result = std::from_chars(begin, end, bits, 16);
    if (result.ec != std::errc() || result.ptr != end) {
      return std::nullopt;
    }

    if (type == bir::TypeKind::F64) {
      return bir::Value::immediate_f64_bits(bits);
    }

    if (bits <= 0xFFFF'FFFFULL) {
      return bir::Value::immediate_f32_bits(static_cast<std::uint32_t>(bits));
    }

    double as_double = 0.0;
    std::memcpy(&as_double, &bits, sizeof(as_double));
    const float as_float = static_cast<float>(as_double);

    std::uint32_t float_bits = 0;
    std::memcpy(&float_bits, &as_float, sizeof(float_bits));
    return bir::Value::immediate_f32_bits(float_bits);
  }

  const auto parsed = parse_i64(trimmed);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(*parsed != 0);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*parsed));
    case bir::TypeKind::I16:
      return bir::Value::immediate_i16(static_cast<std::int16_t>(*parsed));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*parsed));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*parsed);
    default:
      return std::nullopt;
  }
}

namespace {

std::optional<bir::Value> lower_zero_initializer_value(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(false);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(0);
    case bir::TypeKind::I16:
      return bir::Value::immediate_i16(0);
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(0);
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(0);
    case bir::TypeKind::F32:
      return bir::Value::immediate_f32_bits(0u);
    case bir::TypeKind::F64:
      return bir::Value::immediate_f64_bits(0u);
    case bir::TypeKind::Ptr:
      return bir::Value{
          .kind = bir::Value::Kind::Immediate,
          .type = bir::TypeKind::Ptr,
          .immediate = 0,
          .immediate_bits = 0,
      };
    default:
      return std::nullopt;
  }
}

bool append_zero_integer_array_initializer(std::string_view type_text,
                                           std::vector<bir::Value>* out) {
  const auto scalar_type = lower_integer_type(type_text);
  if (scalar_type.has_value()) {
    const auto zero_value = lower_zero_initializer_value(*scalar_type);
    if (!zero_value.has_value()) {
      return false;
    }
    out->push_back(*zero_value);
    return true;
  }

  const auto parsed_layer = parse_integer_array_type(type_text);
  if (!parsed_layer.has_value() || parsed_layer->extents.empty()) {
    return false;
  }
  const auto element_type = peel_integer_array_layer(type_text);
  if (!element_type.has_value()) {
    return false;
  }
  for (std::size_t index = 0; index < parsed_layer->extents.front(); ++index) {
    if (!append_zero_integer_array_initializer(*element_type, out)) {
      return false;
    }
  }
  return true;
}

bool lower_integer_array_initializer_recursive(std::string_view init_text,
                                               std::string_view type_text,
                                               std::vector<bir::Value>* out) {
  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init.empty()) {
    return false;
  }

  if (trimmed_init == "zeroinitializer") {
    return append_zero_integer_array_initializer(type_text, out);
  }

  if (const auto scalar_type = lower_integer_type(type_text); scalar_type.has_value()) {
    std::string_view scalar_init = trimmed_init;
    const auto space = trimmed_init.find(' ');
    if (space != std::string_view::npos && trimmed_init.substr(0, space) == type_text) {
      scalar_init = trimmed_init.substr(space + 1);
    }
    const auto value = lower_global_initializer(scalar_init, *scalar_type);
    if (!value.has_value()) {
      return false;
    }
    out->push_back(*value);
    return true;
  }

  const auto layer = peel_integer_array_layer(type_text);
  const auto array_type = parse_integer_array_type(type_text);
  if (!layer.has_value() || !array_type.has_value() || array_type->extents.empty() ||
      trimmed_init.front() != '[' || trimmed_init.back() != ']') {
    return false;
  }

  const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
  const auto items = split_top_level_initializer_items(body);
  if (items.size() > array_type->extents.front()) {
    return false;
  }

  for (const auto item : items) {
    const auto trimmed_item = c4c::codegen::lir::trim_lir_arg_text(item);
    if (trimmed_item.empty()) {
      return false;
    }
    if (trimmed_item.size() <= layer->size() ||
        trimmed_item.substr(0, layer->size()) != *layer ||
        trimmed_item[layer->size()] != ' ') {
      return false;
    }
    if (!lower_integer_array_initializer_recursive(
            trimmed_item.substr(layer->size() + 1), *layer, out)) {
      return false;
    }
  }

  for (std::size_t index = items.size(); index < array_type->extents.front(); ++index) {
    if (!append_zero_integer_array_initializer(*layer, out)) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::optional<std::vector<bir::Value>> lower_integer_array_initializer(std::string_view init_text,
                                                                       std::string_view type_text) {
  if (const auto lowered_bytes = lower_llvm_byte_string_initializer(init_text, type_text);
      lowered_bytes.has_value()) {
    return lowered_bytes;
  }

  std::vector<bir::Value> lowered;
  if (!lower_integer_array_initializer_recursive(init_text, type_text, &lowered)) {
    return std::nullopt;
  }
  return lowered;
}

std::string_view strip_typed_initializer_prefix(std::string_view init_text,
                                                std::string_view expected_type_text) {
  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  const auto trimmed_type = c4c::codegen::lir::trim_lir_arg_text(expected_type_text);
  if (trimmed_type.empty() || trimmed_init.size() <= trimmed_type.size() ||
      trimmed_init.substr(0, trimmed_type.size()) != trimmed_type ||
      trimmed_init[trimmed_type.size()] != ' ') {
    return trimmed_init;
  }
  return c4c::codegen::lir::trim_lir_arg_text(trimmed_init.substr(trimmed_type.size() + 1));
}

namespace {

bool append_zero_aggregate_initializer(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::vector<bir::Value>* out,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets,
    std::size_t byte_offset) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Scalar: {
      if (layout.scalar_type == bir::TypeKind::F128) {
        for (std::size_t index = 0; index < layout.size_bytes; ++index) {
          out->push_back(bir::Value::immediate_i8(0));
        }
        return true;
      }
      const auto zero_value = lower_zero_initializer_value(layout.scalar_type);
      if (!zero_value.has_value()) {
        return false;
      }
      out->push_back(*zero_value);
      if (layout.scalar_type == bir::TypeKind::Ptr) {
        pointer_offsets->erase(byte_offset);
      }
      return true;
    }
    case AggregateTypeLayout::Kind::Array:
      for (std::size_t index = 0; index < layout.array_count; ++index) {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            !append_zero_aggregate_initializer(layout.element_type_text,
                                              type_decls,
                                              out,
                                              pointer_offsets,
                                              byte_offset + index * element_layout.size_bytes)) {
          return false;
        }
      }
      return true;
    case AggregateTypeLayout::Kind::Struct:
      for (const auto& field : layout.fields) {
        if (!append_zero_aggregate_initializer(field.type_text,
                                              type_decls,
                                              out,
                                              pointer_offsets,
                                              byte_offset + field.byte_offset)) {
          return false;
        }
      }
      return true;
    default:
      return false;
  }
}

bool lower_aggregate_initializer_recursive(
    std::string_view init_text,
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::vector<bir::Value>* out,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets,
    std::size_t byte_offset) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid) {
    return false;
  }

  auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init.empty()) {
    return false;
  }
  trimmed_init = strip_typed_initializer_prefix(trimmed_init, type_text);

  if (trimmed_init == "zeroinitializer") {
    return append_zero_aggregate_initializer(
        type_text, type_decls, out, pointer_offsets, byte_offset);
  }

  if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
    if (layout.scalar_type == bir::TypeKind::F128) {
      return append_x86_fp80_initializer_bytes(trimmed_init, out);
    }
    if (layout.scalar_type == bir::TypeKind::Ptr) {
      if (const auto address = parse_global_address_initializer(trimmed_init, type_decls);
          address.has_value()) {
        pointer_offsets->emplace(byte_offset, *address);
        out->push_back(bir::Value::named(bir::TypeKind::Ptr, "@" + address->global_name));
        return true;
      }
    }
    const auto value = lower_global_initializer(trimmed_init, layout.scalar_type);
    if (!value.has_value()) {
      return false;
    }
    if (layout.scalar_type == bir::TypeKind::Ptr) {
      pointer_offsets->erase(byte_offset);
    }
    out->push_back(*value);
    return true;
  }

  if (layout.kind == AggregateTypeLayout::Kind::Array) {
    if (const auto integer_array_elements = lower_integer_array_initializer(trimmed_init, type_text);
        integer_array_elements.has_value()) {
      out->insert(out->end(), integer_array_elements->begin(), integer_array_elements->end());
      return true;
    }
    if (trimmed_init.front() != '[' || trimmed_init.back() != ']') {
      return false;
    }
    const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
    if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        element_layout.size_bytes == 0) {
      return false;
    }
    const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
    const auto items = split_top_level_initializer_items(body);
    if (items.size() > layout.array_count) {
      return false;
    }
    for (std::size_t index = 0; index < items.size(); ++index) {
      if (!lower_aggregate_initializer_recursive(items[index],
                                                 layout.element_type_text,
                                                 type_decls,
                                                 out,
                                                 pointer_offsets,
                                                 byte_offset + index * element_layout.size_bytes)) {
        return false;
      }
    }
    for (std::size_t index = items.size(); index < layout.array_count; ++index) {
      if (!append_zero_aggregate_initializer(layout.element_type_text,
                                             type_decls,
                                             out,
                                             pointer_offsets,
                                             byte_offset + index * element_layout.size_bytes)) {
        return false;
      }
    }
    return true;
  }

  if (trimmed_init.front() != '{' || trimmed_init.back() != '}') {
    return false;
  }
  const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
  const auto items = split_top_level_initializer_items(body);
  if (items.size() > layout.fields.size()) {
    return false;
  }
  for (std::size_t index = 0; index < items.size(); ++index) {
    if (!lower_aggregate_initializer_recursive(items[index],
                                               layout.fields[index].type_text,
                                               type_decls,
                                               out,
                                               pointer_offsets,
                                               byte_offset + layout.fields[index].byte_offset)) {
      return false;
    }
  }
  for (std::size_t index = items.size(); index < layout.fields.size(); ++index) {
    if (!append_zero_aggregate_initializer(layout.fields[index].type_text,
                                           type_decls,
                                           out,
                                           pointer_offsets,
                                           byte_offset + layout.fields[index].byte_offset)) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::optional<std::vector<bir::Value>> lower_aggregate_initializer(
    std::string_view init_text,
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets) {
  std::vector<bir::Value> lowered;
  if (!lower_aggregate_initializer_recursive(
          init_text, type_text, type_decls, &lowered, pointer_offsets, 0)) {
    return std::nullopt;
  }
  return lowered;
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
