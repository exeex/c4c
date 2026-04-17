#include "lir_to_bir.hpp"

#include <algorithm>
#include <charconv>

namespace c4c::backend::lir_to_bir_detail {

std::optional<bir::TypeKind> lower_integer_type(std::string_view text);

namespace {

std::optional<std::pair<std::size_t, std::string_view>> parse_integer_array_layer(
    std::string_view text) {
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

  return std::pair<std::size_t, std::string_view>{
      static_cast<std::size_t>(*count),
      text.substr(x_pos + 3, text.size() - x_pos - 4),
  };
}

std::size_t align_up(std::size_t value, std::size_t align_bytes) {
  if (align_bytes <= 1) {
    return value;
  }
  const auto remainder = value % align_bytes;
  return remainder == 0 ? value : value + (align_bytes - remainder);
}

std::optional<std::string_view> resolve_type_decl_body(std::string_view text,
                                                       const TypeDeclMap& type_decls) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty() || trimmed.front() != '%') {
    return std::nullopt;
  }
  const auto it = type_decls.find(std::string(trimmed));
  if (it == type_decls.end()) {
    return std::nullopt;
  }
  return std::string_view(it->second);
}

std::optional<bir::TypeKind> lower_scalar_storage_type(std::string_view text) {
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

TypeDeclMap build_type_decl_map(const std::vector<std::string>& type_decls) {
  TypeDeclMap lowered;
  lowered.reserve(type_decls.size());
  for (const auto& decl : type_decls) {
    const auto eq = decl.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const auto type_kw = decl.find("type", eq);
    if (type_kw == std::string::npos) {
      continue;
    }
    const auto name = std::string(c4c::codegen::lir::trim_lir_arg_text(decl.substr(0, eq)));
    const auto body =
        std::string(c4c::codegen::lir::trim_lir_arg_text(decl.substr(type_kw + 4)));
    if (!name.empty() && !body.empty()) {
      lowered.emplace(name, body);
    }
  }
  return lowered;
}

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<bir::TypeKind> lower_integer_type(std::string_view text) {
  if (text == "ptr") {
    return bir::TypeKind::Ptr;
  }
  if (text == "i1") {
    return bir::TypeKind::I1;
  }
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i16") {
    return bir::TypeKind::I16;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  if (text == "void") {
    return bir::TypeKind::Void;
  }
  return std::nullopt;
}

std::size_t type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I16:
      return 2;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
      return 8;
    default:
      return 0;
  }
}

std::vector<std::string_view> split_top_level_initializer_items(std::string_view text) {
  std::vector<std::string_view> items;
  std::size_t item_start = 0;
  int depth = 0;
  for (std::size_t index = 0; index < text.size(); ++index) {
    const char ch = text[index];
    if (ch == '[' || ch == '{') {
      ++depth;
    } else if (ch == ']' || ch == '}') {
      --depth;
    } else if (ch == ',' && depth == 0) {
      items.push_back(text.substr(item_start, index - item_start));
      item_start = index + 1;
    }
  }
  items.push_back(text.substr(item_start));
  return items;
}

std::optional<ParsedTypedOperand> parse_typed_operand(std::string_view text) {
  const auto space = text.find(' ');
  if (space == std::string_view::npos || space == 0 || space + 1 >= text.size()) {
    return std::nullopt;
  }
  return ParsedTypedOperand{
      .type_text = std::string(text.substr(0, space)),
      .operand = c4c::codegen::lir::LirOperand(std::string(text.substr(space + 1))),
  };
}

std::optional<std::int64_t> resolve_index_operand(const c4c::codegen::lir::LirOperand& operand,
                                                  const ValueMap& value_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Immediate ||
      operand.kind() == c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return parse_i64(operand.str());
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto alias = value_aliases.find(operand.str());
  if (alias == value_aliases.end() || alias->second.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  return alias->second.immediate;
}

AggregateTypeLayout compute_aggregate_type_layout(std::string_view text,
                                                  const TypeDeclMap& type_decls) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty()) {
    return {};
  }

  if (const auto scalar_type = lower_scalar_storage_type(trimmed); scalar_type.has_value()) {
    return AggregateTypeLayout{
        .kind = AggregateTypeLayout::Kind::Scalar,
        .scalar_type = *scalar_type,
        .size_bytes = type_size_bytes(*scalar_type),
        .align_bytes = type_size_bytes(*scalar_type),
    };
  }

  if (const auto resolved = resolve_type_decl_body(trimmed, type_decls); resolved.has_value()) {
    return compute_aggregate_type_layout(*resolved, type_decls);
  }

  if (const auto layer = parse_integer_array_layer(trimmed); layer.has_value()) {
    const auto element_layout = compute_aggregate_type_layout(layer->second, type_decls);
    if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        element_layout.size_bytes == 0 || element_layout.align_bytes == 0) {
      return {};
    }
    return AggregateTypeLayout{
        .kind = AggregateTypeLayout::Kind::Array,
        .size_bytes = layer->first * element_layout.size_bytes,
        .align_bytes = element_layout.align_bytes,
        .array_count = layer->first,
        .element_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(layer->second)),
    };
  }

  if (trimmed.size() < 2 || trimmed.front() != '{' || trimmed.back() != '}') {
    return {};
  }

  const auto body = trimmed.substr(1, trimmed.size() - 2);
  const auto field_items = split_top_level_initializer_items(body);
  AggregateTypeLayout layout;
  layout.kind = AggregateTypeLayout::Kind::Struct;
  std::size_t current_offset = 0;
  std::size_t struct_align = 1;
  for (const auto item : field_items) {
    const auto field_type = c4c::codegen::lir::trim_lir_arg_text(item);
    if (field_type.empty()) {
      return {};
    }
    const auto field_layout = compute_aggregate_type_layout(field_type, type_decls);
    if (field_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        field_layout.size_bytes == 0 || field_layout.align_bytes == 0) {
      return {};
    }
    current_offset = align_up(current_offset, field_layout.align_bytes);
    layout.fields.push_back(AggregateField{
        .byte_offset = current_offset,
        .type_text = std::string(field_type),
    });
    current_offset += field_layout.size_bytes;
    struct_align = std::max(struct_align, field_layout.align_bytes);
  }

  if (layout.fields.empty()) {
    return {};
  }
  layout.align_bytes = struct_align;
  layout.size_bytes = align_up(current_offset, struct_align);
  return layout;
}

}  // namespace c4c::backend::lir_to_bir_detail
