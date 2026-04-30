#include "lowering.hpp"

#include <array>
#include <charconv>
#include <cstring>
#include <utility>

namespace c4c::backend::lir_to_bir_detail {

namespace {

std::optional<std::string> parse_global_symbol_initializer(std::string_view text) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.size() < 2 || trimmed.front() != '@') {
    return std::nullopt;
  }
  return std::string(trimmed.substr(1));
}

BackendAggregateLayoutLookup lookup_global_initializer_layout_result(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (structured_layouts != nullptr) {
    return lookup_backend_aggregate_type_layout_result(type_text, type_decls, *structured_layouts);
  }
  return BackendAggregateLayoutLookup{
      .layout = compute_aggregate_type_layout(type_text, type_decls),
      .used_structured_layout = false,
      .used_legacy_fallback = true,
      .structured_text_mismatch = false,
  };
}

AggregateTypeLayout lookup_global_initializer_layout(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  return lookup_global_initializer_layout_result(type_text, type_decls, structured_layouts).layout;
}

std::optional<GlobalAddress> parse_global_gep_initializer(std::string_view text,
                                                          const TypeDeclMap& type_decls,
                                                          const BackendStructuredLayoutTable*
                                                              structured_layouts) {
  constexpr std::string_view kPrefix = "getelementptr inbounds (";

  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.size() <= kPrefix.size() || trimmed.substr(0, kPrefix.size()) != kPrefix ||
      trimmed.back() != ')') {
    return std::nullopt;
  }

  const auto body = trimmed.substr(kPrefix.size(), trimmed.size() - kPrefix.size() - 1);
  const auto ptr_pos = body.find(", ptr @");
  if (ptr_pos == std::string_view::npos) {
    return std::nullopt;
  }

  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(body.substr(0, ptr_pos));
  auto remainder = body.substr(ptr_pos + 7);
  const auto global_end = remainder.find(',');

  std::string global_name;
  if (global_end == std::string_view::npos) {
    global_name = std::string(c4c::codegen::lir::trim_lir_arg_text(remainder));
    remainder = std::string_view();
  } else {
    global_name =
        std::string(c4c::codegen::lir::trim_lir_arg_text(remainder.substr(0, global_end)));
    remainder = remainder.substr(global_end + 1);
  }
  if (global_name.empty()) {
    return std::nullopt;
  }

  std::size_t byte_offset = 0;
  bool saw_index = false;
  while (!remainder.empty()) {
    remainder = c4c::codegen::lir::trim_lir_arg_text(remainder);
    if (remainder.empty()) {
      break;
    }

    const auto comma = remainder.find(',');
    const auto index_text =
        c4c::codegen::lir::trim_lir_arg_text(comma == std::string_view::npos
                                                 ? remainder
                                                 : remainder.substr(0, comma));
    const auto index = parse_typed_operand(index_text);
    if (!index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(index->operand, ValueMap{});
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout =
        lookup_global_initializer_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_index) {
      saw_index = true;
      if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
        byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
        if (comma != std::string_view::npos) {
          return std::nullopt;
        }
        return GlobalAddress{
            .global_name = std::move(global_name),
            .value_type = layout.scalar_type,
            .byte_offset = byte_offset,
        };
      }
      if (*index_value != 0) {
        return std::nullopt;
      }
      if (comma == std::string_view::npos) {
        break;
      }
      remainder = remainder.substr(comma + 1);
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            lookup_global_initializer_layout(
                layout.element_type_text, type_decls, structured_layouts);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      case AggregateTypeLayout::Kind::Scalar:
        if (comma != std::string_view::npos) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
        return GlobalAddress{
            .global_name = std::move(global_name),
            .value_type = layout.scalar_type,
            .byte_offset = byte_offset,
        };
      default:
        return std::nullopt;
    }
    if (comma == std::string_view::npos) {
      break;
    }
    remainder = remainder.substr(comma + 1);
  }

  const auto leaf_layout =
      lookup_global_initializer_layout(current_type, type_decls, structured_layouts);
  if (leaf_layout.kind == AggregateTypeLayout::Kind::Scalar) {
    return GlobalAddress{
        .global_name = std::move(global_name),
        .value_type = leaf_layout.scalar_type,
        .byte_offset = byte_offset,
    };
  }

  return std::nullopt;
}

}  // namespace

std::optional<GlobalAddress> parse_global_address_initializer_impl(
    std::string_view text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (const auto symbol_name = parse_global_symbol_initializer(text); symbol_name.has_value()) {
    return GlobalAddress{
        .global_name = *symbol_name,
        .value_type = bir::TypeKind::Void,
        .byte_offset = 0,
    };
  }
  return parse_global_gep_initializer(text, type_decls, structured_layouts);
}

std::optional<GlobalAddress> parse_global_address_initializer(std::string_view text,
                                                              const TypeDeclMap& type_decls) {
  return parse_global_address_initializer_impl(text, type_decls, nullptr);
}

std::optional<GlobalAddress> parse_global_address_initializer(
    std::string_view text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return parse_global_address_initializer_impl(text, type_decls, &structured_layouts);
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
    const BackendStructuredLayoutTable* structured_layouts,
    std::vector<bir::Value>* out,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets,
    std::size_t byte_offset) {
  const auto layout = lookup_global_initializer_layout(type_text, type_decls, structured_layouts);
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
            lookup_global_initializer_layout(
                layout.element_type_text, type_decls, structured_layouts);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            !append_zero_aggregate_initializer(layout.element_type_text,
                                              type_decls,
                                              structured_layouts,
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
                                              structured_layouts,
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
    const BackendStructuredLayoutTable* structured_layouts,
    std::vector<bir::Value>* out,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets,
    std::size_t byte_offset) {
  const auto layout = lookup_global_initializer_layout(type_text, type_decls, structured_layouts);
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
        type_text, type_decls, structured_layouts, out, pointer_offsets, byte_offset);
  }

  if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
    if (layout.scalar_type == bir::TypeKind::F128) {
      return append_x86_fp80_initializer_bytes(trimmed_init, out);
    }
    if (layout.scalar_type == bir::TypeKind::Ptr) {
      const auto address = structured_layouts != nullptr
                               ? parse_global_address_initializer(
                                     trimmed_init, type_decls, *structured_layouts)
                               : parse_global_address_initializer(trimmed_init, type_decls);
      if (address.has_value()) {
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
    const auto element_layout =
        lookup_global_initializer_layout(layout.element_type_text, type_decls, structured_layouts);
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
                                                 structured_layouts,
                                                 out,
                                                 pointer_offsets,
                                                 byte_offset + index * element_layout.size_bytes)) {
        return false;
      }
    }
    for (std::size_t index = items.size(); index < layout.array_count; ++index) {
      if (!append_zero_aggregate_initializer(layout.element_type_text,
                                             type_decls,
                                             structured_layouts,
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
                                               structured_layouts,
                                               out,
                                               pointer_offsets,
                                               byte_offset + layout.fields[index].byte_offset)) {
      return false;
    }
  }
  for (std::size_t index = items.size(); index < layout.fields.size(); ++index) {
    if (!append_zero_aggregate_initializer(layout.fields[index].type_text,
                                           type_decls,
                                           structured_layouts,
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
          init_text, type_text, type_decls, nullptr, &lowered, pointer_offsets, 0)) {
    return std::nullopt;
  }
  return lowered;
}

std::optional<std::vector<bir::Value>> lower_aggregate_initializer(
    std::string_view init_text,
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets) {
  std::vector<bir::Value> lowered;
  if (!lower_aggregate_initializer_recursive(
          init_text, type_text, type_decls, &structured_layouts, &lowered, pointer_offsets, 0)) {
    return std::nullopt;
  }
  return lowered;
}

}  // namespace c4c::backend::lir_to_bir_detail
