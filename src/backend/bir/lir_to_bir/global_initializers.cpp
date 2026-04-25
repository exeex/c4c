#include "lowering.hpp"

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

std::optional<GlobalAddress> parse_global_gep_initializer(std::string_view text,
                                                          const TypeDeclMap& type_decls) {
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

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
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
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
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

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
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

std::optional<GlobalAddress> parse_global_address_initializer(std::string_view text,
                                                              const TypeDeclMap& type_decls) {
  if (const auto symbol_name = parse_global_symbol_initializer(text); symbol_name.has_value()) {
    return GlobalAddress{
        .global_name = *symbol_name,
        .value_type = bir::TypeKind::Void,
        .byte_offset = 0,
    };
  }
  return parse_global_gep_initializer(text, type_decls);
}

}  // namespace c4c::backend::lir_to_bir_detail
