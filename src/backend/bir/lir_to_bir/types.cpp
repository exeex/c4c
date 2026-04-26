#include "lowering.hpp"

#include <algorithm>
#include <charconv>

namespace c4c::backend::lir_to_bir_detail {

std::optional<bir::TypeKind> lower_integer_type(std::string_view text);

namespace {

using c4c::codegen::lir::LirStructDecl;

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
  if (text == "x86_fp80" || text == "f128") {
    return bir::TypeKind::F128;
  }
  return std::nullopt;
}

bool aggregate_layouts_match(const AggregateTypeLayout& lhs,
                             const AggregateTypeLayout& rhs) {
  if (lhs.kind != rhs.kind || lhs.scalar_type != rhs.scalar_type ||
      lhs.size_bytes != rhs.size_bytes || lhs.align_bytes != rhs.align_bytes ||
      lhs.array_count != rhs.array_count ||
      lhs.element_type_text != rhs.element_type_text ||
      lhs.fields.size() != rhs.fields.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.fields.size(); ++index) {
    if (lhs.fields[index].byte_offset != rhs.fields[index].byte_offset ||
        lhs.fields[index].type_text != rhs.fields[index].type_text) {
      return false;
    }
  }
  return true;
}

std::string layout_field_summary(const AggregateTypeLayout& layout) {
  std::string summary = "[";
  for (std::size_t index = 0; index < layout.fields.size(); ++index) {
    if (index != 0) {
      summary += ", ";
    }
    summary += layout.fields[index].type_text;
    summary += "@";
    summary += std::to_string(layout.fields[index].byte_offset);
  }
  summary += "]";
  return summary;
}

AggregateTypeLayout compute_structured_layout_from_type(
    std::string_view text,
    const std::unordered_map<std::string, const LirStructDecl*>& structured_decls,
    std::unordered_set<std::string>* active);

AggregateTypeLayout compute_structured_decl_layout(
    const LirStructDecl& decl,
    const std::unordered_map<std::string, const LirStructDecl*>& structured_decls,
    std::unordered_set<std::string>* active) {
  AggregateTypeLayout layout;
  if (decl.is_opaque) {
    return layout;
  }

  layout.kind = AggregateTypeLayout::Kind::Struct;
  std::size_t current_offset = 0;
  std::size_t struct_align = 1;
  for (const auto& field : decl.fields) {
    const auto field_type = c4c::codegen::lir::trim_lir_arg_text(field.type.str());
    if (field_type.empty()) {
      return {};
    }
    const auto field_layout =
        compute_structured_layout_from_type(field_type, structured_decls, active);
    if (field_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        field_layout.align_bytes == 0 ||
        (field_layout.kind == AggregateTypeLayout::Kind::Scalar &&
         field_layout.size_bytes == 0)) {
      return {};
    }
    if (!decl.is_packed) {
      current_offset = align_up(current_offset, field_layout.align_bytes);
    }
    layout.fields.push_back(AggregateField{
        .byte_offset = current_offset,
        .type_text = std::string(field_type),
    });
    current_offset += field_layout.size_bytes;
    if (!decl.is_packed) {
      struct_align = std::max(struct_align, field_layout.align_bytes);
    }
  }

  layout.align_bytes = struct_align;
  layout.size_bytes = decl.is_packed ? current_offset : align_up(current_offset, struct_align);
  return layout;
}

AggregateTypeLayout compute_structured_layout_from_type(
    std::string_view text,
    const std::unordered_map<std::string, const LirStructDecl*>& structured_decls,
    std::unordered_set<std::string>* active) {
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

  if (trimmed.front() == '%') {
    const std::string type_name(trimmed);
    if (!active->insert(type_name).second) {
      return {};
    }
    const auto structured_it = structured_decls.find(type_name);
    if (structured_it == structured_decls.end()) {
      active->erase(type_name);
      return {};
    }
    auto layout = compute_structured_decl_layout(*structured_it->second, structured_decls, active);
    active->erase(type_name);
    return layout;
  }

  if (const auto layer = parse_integer_array_layer(trimmed); layer.has_value()) {
    const auto element_layout =
        compute_structured_layout_from_type(layer->second, structured_decls, active);
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

  LirStructDecl inline_struct;
  const auto body = trimmed.substr(1, trimmed.size() - 2);
  for (const auto item : split_top_level_initializer_items(body)) {
    const auto field_type = c4c::codegen::lir::trim_lir_arg_text(item);
    if (field_type.empty()) {
      return {};
    }
    inline_struct.fields.push_back({c4c::codegen::lir::LirTypeRef(std::string(field_type))});
  }
  return compute_structured_decl_layout(inline_struct, structured_decls, active);
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

BackendStructuredLayoutTable build_backend_structured_layout_table(
    const std::vector<c4c::codegen::lir::LirStructDecl>& struct_decls,
    const c4c::StructNameTable& struct_names,
    const TypeDeclMap& legacy_type_decls) {
  std::unordered_map<std::string, const LirStructDecl*> structured_decls;
  structured_decls.reserve(struct_decls.size());
  for (const auto& decl : struct_decls) {
    const std::string_view name = struct_names.spelling(decl.name_id);
    if (!name.empty()) {
      structured_decls.emplace(std::string(name), &decl);
    }
  }

  BackendStructuredLayoutTable table;
  table.reserve(structured_decls.size());
  for (const auto& [name, decl] : structured_decls) {
    std::unordered_set<std::string> active;
    active.insert(name);

    BackendStructuredLayoutEntry entry;
    entry.type_name = name;
    entry.structured_found = true;
    entry.structured_layout =
        compute_structured_decl_layout(*decl, structured_decls, &active);

    const auto legacy_it = legacy_type_decls.find(name);
    entry.legacy_found = legacy_it != legacy_type_decls.end();
    if (entry.legacy_found) {
      entry.legacy_layout = compute_aggregate_type_layout(name, legacy_type_decls);
    }

    entry.parity_checked =
        entry.legacy_found &&
        entry.structured_layout.kind != AggregateTypeLayout::Kind::Invalid &&
        entry.legacy_layout.kind != AggregateTypeLayout::Kind::Invalid;
    entry.parity_matches =
        entry.parity_checked &&
        aggregate_layouts_match(entry.structured_layout, entry.legacy_layout);
    table.emplace(name, std::move(entry));
  }
  return table;
}

bir::StructuredTypeSpellingContext build_bir_structured_type_spelling_context(
    const std::vector<c4c::codegen::lir::LirStructDecl>& struct_decls,
    const c4c::StructNameTable& struct_names) {
  bir::StructuredTypeSpellingContext context;
  context.declarations.reserve(struct_decls.size());
  for (const auto& decl : struct_decls) {
    const std::string_view name = struct_names.spelling(decl.name_id);
    if (name.empty()) {
      continue;
    }

    bir::StructuredTypeDeclSpelling lowered_decl;
    lowered_decl.name = std::string(name);
    lowered_decl.fields.reserve(decl.fields.size());
    lowered_decl.is_packed = decl.is_packed;
    lowered_decl.is_opaque = decl.is_opaque;
    for (const auto& field : decl.fields) {
      lowered_decl.fields.push_back(bir::StructuredTypeFieldSpelling{
          .type_name = std::string(c4c::codegen::lir::trim_lir_arg_text(field.type.str())),
      });
    }
    context.declarations.push_back(std::move(lowered_decl));
  }
  return context;
}

void report_backend_structured_layout_parity_notes(
    BirLoweringContext& context,
    const BackendStructuredLayoutTable& structured_layouts) {
  for (const auto& [type_name, entry] : structured_layouts) {
    if (!entry.parity_checked || entry.parity_matches) {
      continue;
    }

    std::string message = "structured backend layout parity mismatch for ";
    message += type_name;
    message += ": legacy size ";
    message += std::to_string(entry.legacy_layout.size_bytes);
    message += ", align ";
    message += std::to_string(entry.legacy_layout.align_bytes);
    message += ", fields ";
    message += layout_field_summary(entry.legacy_layout);
    message += "; structured size ";
    message += std::to_string(entry.structured_layout.size_bytes);
    message += ", align ";
    message += std::to_string(entry.structured_layout.align_bytes);
    message += ", fields ";
    message += layout_field_summary(entry.structured_layout);
    context.note("module", std::move(message));
  }
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
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    default:
      return 0;
  }
}

std::vector<std::string_view> split_top_level_initializer_items(std::string_view text) {
  std::vector<std::string_view> items;
  if (c4c::codegen::lir::trim_lir_arg_text(text).empty()) {
    return items;
  }
  std::size_t item_start = 0;
  int depth = 0;
  for (std::size_t index = 0; index < text.size(); ++index) {
    const char ch = text[index];
    if (ch == '[' || ch == '{' || ch == '(' || ch == '<') {
      ++depth;
    } else if (ch == ']' || ch == '}' || ch == ')' || ch == '>') {
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

  bool is_packed_struct = false;
  std::string_view body;
  if (trimmed.size() >= 4 && trimmed.substr(0, 2) == "<{" &&
      trimmed.substr(trimmed.size() - 2) == "}>") {
    is_packed_struct = true;
    body = trimmed.substr(2, trimmed.size() - 4);
  } else if (trimmed.size() >= 2 && trimmed.front() == '{' && trimmed.back() == '}') {
    body = trimmed.substr(1, trimmed.size() - 2);
  } else {
    return {};
  }

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
    if (field_layout.kind == AggregateTypeLayout::Kind::Invalid || field_layout.align_bytes == 0 ||
        (field_layout.kind == AggregateTypeLayout::Kind::Scalar && field_layout.size_bytes == 0)) {
      return {};
    }
    if (!is_packed_struct) {
      current_offset = align_up(current_offset, field_layout.align_bytes);
    }
    layout.fields.push_back(AggregateField{
        .byte_offset = current_offset,
        .type_text = std::string(field_type),
    });
    current_offset += field_layout.size_bytes;
    if (!is_packed_struct) {
      struct_align = std::max(struct_align, field_layout.align_bytes);
    }
  }

  layout.align_bytes = struct_align;
  layout.size_bytes = is_packed_struct ? current_offset : align_up(current_offset, struct_align);
  return layout;
}

AggregateTypeLayout lookup_backend_aggregate_type_layout(
    std::string_view text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (!trimmed.empty() && trimmed.front() == '%') {
    const auto structured_it = structured_layouts.find(std::string(trimmed));
    if (structured_it != structured_layouts.end()) {
      const auto& entry = structured_it->second;
      if (entry.structured_layout.kind != AggregateTypeLayout::Kind::Invalid &&
          (!entry.legacy_found || (entry.parity_checked && entry.parity_matches))) {
        return entry.structured_layout;
      }
    }
  }

  return compute_aggregate_type_layout(trimmed, type_decls);
}

}  // namespace c4c::backend::lir_to_bir_detail
