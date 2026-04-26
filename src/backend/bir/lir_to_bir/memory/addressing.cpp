#include "../lowering.hpp"
#include "memory_helpers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using DynamicGlobalAggregateArrayAccess = BirFunctionLowerer::DynamicGlobalAggregateArrayAccess;
using DynamicGlobalPointerArrayAccess = BirFunctionLowerer::DynamicGlobalPointerArrayAccess;
using DynamicGlobalScalarArrayAccess = BirFunctionLowerer::DynamicGlobalScalarArrayAccess;
using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using DynamicLocalPointerArrayAccess = BirFunctionLowerer::DynamicLocalPointerArrayAccess;
using GlobalAddress = BirFunctionLowerer::GlobalAddress;
using LocalPointerArrayBase = BirFunctionLowerer::LocalPointerArrayBase;
using PointerAddress = BirFunctionLowerer::PointerAddress;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::lookup_backend_aggregate_type_layout;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

namespace {

using BackendStructuredLayoutTable = lir_to_bir_detail::BackendStructuredLayoutTable;

BirFunctionLowerer::AggregateTypeLayout lookup_addressing_layout(
    std::string_view type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (structured_layouts != nullptr) {
    return lookup_backend_aggregate_type_layout(type_text, type_decls, *structured_layouts);
  }
  return compute_aggregate_type_layout(type_text, type_decls);
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
find_repeated_aggregate_extent_at_offset_impl(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    bool include_struct_field_runs) {
  const auto projection = structured_layouts != nullptr
                              ? resolve_aggregate_byte_offset_projection(
                                    type_text, target_offset, type_decls, *structured_layouts)
                              : resolve_aggregate_byte_offset_projection(
                                    type_text, target_offset, type_decls);
  if (!projection.has_value()) {
    return std::nullopt;
  }

  switch (projection->kind) {
    case AggregateByteOffsetProjection::Kind::ArrayElement:
      if (projection->byte_offset_within_child == 0 &&
          c4c::codegen::lir::trim_lir_arg_text(projection->child_type_text) ==
              c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
        return BirFunctionLowerer::AggregateArrayExtent{
            .element_count = projection->layout.array_count - projection->child_index,
            .element_stride_bytes = projection->child_stride_bytes,
        };
      }
      return find_repeated_aggregate_extent_at_offset_impl(
          projection->child_type_text,
          projection->byte_offset_within_child,
          repeated_type_text,
          type_decls,
          structured_layouts,
          include_struct_field_runs);
    case AggregateByteOffsetProjection::Kind::StructField:
      if (include_struct_field_runs && projection->byte_offset_within_child == 0 &&
          projection->child_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid &&
          projection->child_layout.size_bytes != 0 &&
          c4c::codegen::lir::trim_lir_arg_text(projection->child_type_text) ==
              c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
        std::size_t repeated_count = 0;
        const auto field_begin = projection->child_start_byte_offset;
        for (std::size_t repeated_index = projection->child_index;
             repeated_index < projection->layout.fields.size();
             ++repeated_index) {
          if (c4c::codegen::lir::trim_lir_arg_text(
                  projection->layout.fields[repeated_index].type_text) !=
                  c4c::codegen::lir::trim_lir_arg_text(repeated_type_text) ||
              projection->layout.fields[repeated_index].byte_offset !=
                  field_begin + repeated_count * projection->child_stride_bytes) {
            break;
          }
          ++repeated_count;
        }
        if (repeated_count != 0) {
          return BirFunctionLowerer::AggregateArrayExtent{
              .element_count = repeated_count,
              .element_stride_bytes = projection->child_stride_bytes,
          };
        }
      }
      return find_repeated_aggregate_extent_at_offset_impl(projection->child_type_text,
                                                           projection->byte_offset_within_child,
                                                           repeated_type_text,
                                                           type_decls,
                                                           structured_layouts,
                                                           include_struct_field_runs);
    default:
      return std::nullopt;
  }
}

}  // namespace

std::optional<AggregateByteOffsetProjection> resolve_aggregate_byte_offset_projection(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  const auto layout = lookup_addressing_layout(type_text, type_decls, &structured_layouts);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          lookup_addressing_layout(layout.element_type_text, type_decls, &structured_layouts);
      if (element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      return AggregateByteOffsetProjection{
          .kind = AggregateByteOffsetProjection::Kind::ArrayElement,
          .layout = layout,
          .child_layout = element_layout,
          .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
              layout.element_type_text)),
          .child_index = element_index,
          .byte_offset_within_child = target_offset % element_layout.size_bytes,
          .target_byte_offset = target_offset,
          .child_start_byte_offset = element_index * element_layout.size_bytes,
          .child_stride_bytes = element_layout.size_bytes,
      };
    }
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        const auto child_layout =
            lookup_addressing_layout(layout.fields[index].type_text, type_decls, &structured_layouts);
        return AggregateByteOffsetProjection{
            .kind = AggregateByteOffsetProjection::Kind::StructField,
            .layout = layout,
            .child_layout = child_layout,
            .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
                layout.fields[index].type_text)),
            .child_index = index,
            .byte_offset_within_child = target_offset - field_begin,
            .target_byte_offset = target_offset,
            .child_start_byte_offset = field_begin,
            .child_stride_bytes = child_layout.size_bytes,
        };
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<AggregateByteOffsetProjection> resolve_aggregate_child_index_projection(
    std::string_view type_text,
    std::size_t child_index,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  const auto layout = lookup_addressing_layout(type_text, type_decls, &structured_layouts);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      layout.size_bytes == 0) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          lookup_addressing_layout(layout.element_type_text, type_decls, &structured_layouts);
      if (element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
          child_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto child_start_byte_offset = child_index * element_layout.size_bytes;
      return AggregateByteOffsetProjection{
          .kind = AggregateByteOffsetProjection::Kind::ArrayElement,
          .layout = layout,
          .child_layout = element_layout,
          .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
              layout.element_type_text)),
          .child_index = child_index,
          .byte_offset_within_child = 0,
          .target_byte_offset = child_start_byte_offset,
          .child_start_byte_offset = child_start_byte_offset,
          .child_stride_bytes = element_layout.size_bytes,
      };
    }
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Struct: {
      if (child_index >= layout.fields.size()) {
        return std::nullopt;
      }
      const auto child_layout =
          lookup_addressing_layout(layout.fields[child_index].type_text, type_decls, &structured_layouts);
      return AggregateByteOffsetProjection{
          .kind = AggregateByteOffsetProjection::Kind::StructField,
          .layout = layout,
          .child_layout = child_layout,
          .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
              layout.fields[child_index].type_text)),
          .child_index = child_index,
          .byte_offset_within_child = 0,
          .target_byte_offset = layout.fields[child_index].byte_offset,
          .child_start_byte_offset = layout.fields[child_index].byte_offset,
          .child_stride_bytes = child_layout.size_bytes,
      };
    }
    default:
      return std::nullopt;
  }
}

std::optional<AggregateByteOffsetProjection> resolve_aggregate_byte_offset_projection(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      return AggregateByteOffsetProjection{
          .kind = AggregateByteOffsetProjection::Kind::ArrayElement,
          .layout = layout,
          .child_layout = element_layout,
          .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
              layout.element_type_text)),
          .child_index = element_index,
          .byte_offset_within_child = target_offset % element_layout.size_bytes,
          .target_byte_offset = target_offset,
          .child_start_byte_offset = element_index * element_layout.size_bytes,
          .child_stride_bytes = element_layout.size_bytes,
      };
    }
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        const auto child_layout =
            compute_aggregate_type_layout(layout.fields[index].type_text, type_decls);
        return AggregateByteOffsetProjection{
            .kind = AggregateByteOffsetProjection::Kind::StructField,
            .layout = layout,
            .child_layout = child_layout,
            .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
                layout.fields[index].type_text)),
            .child_index = index,
            .byte_offset_within_child = target_offset - field_begin,
            .target_byte_offset = target_offset,
            .child_start_byte_offset = field_begin,
            .child_stride_bytes = child_layout.size_bytes,
        };
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<AggregateByteOffsetProjection> resolve_aggregate_child_index_projection(
    std::string_view type_text,
    std::size_t child_index,
    const BirFunctionLowerer::TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      layout.size_bytes == 0) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
          child_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto child_start_byte_offset = child_index * element_layout.size_bytes;
      return AggregateByteOffsetProjection{
          .kind = AggregateByteOffsetProjection::Kind::ArrayElement,
          .layout = layout,
          .child_layout = element_layout,
          .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
              layout.element_type_text)),
          .child_index = child_index,
          .byte_offset_within_child = 0,
          .target_byte_offset = child_start_byte_offset,
          .child_start_byte_offset = child_start_byte_offset,
          .child_stride_bytes = element_layout.size_bytes,
      };
    }
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Struct: {
      if (child_index >= layout.fields.size()) {
        return std::nullopt;
      }
      const auto child_layout =
          compute_aggregate_type_layout(layout.fields[child_index].type_text, type_decls);
      return AggregateByteOffsetProjection{
          .kind = AggregateByteOffsetProjection::Kind::StructField,
          .layout = layout,
          .child_layout = child_layout,
          .child_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(
              layout.fields[child_index].type_text)),
          .child_index = child_index,
          .byte_offset_within_child = 0,
          .target_byte_offset = layout.fields[child_index].byte_offset,
          .child_start_byte_offset = layout.fields[child_index].byte_offset,
          .child_stride_bytes = child_layout.size_bytes,
      };
    }
    default:
      return std::nullopt;
  }
}

namespace {

bool can_reinterpret_byte_storage_as_type_impl(
    std::string_view storage_type_text,
    std::size_t target_byte_offset,
    std::string_view target_type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  const auto storage_layout =
      lookup_addressing_layout(storage_type_text, type_decls, structured_layouts);
  const auto target_layout =
      lookup_addressing_layout(target_type_text, type_decls, structured_layouts);
  if (storage_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Array ||
      target_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      storage_layout.size_bytes == 0 || target_layout.size_bytes == 0 ||
      target_byte_offset != 0 || storage_layout.size_bytes != target_layout.size_bytes) {
    return false;
  }
  const auto element_layout =
      lookup_addressing_layout(storage_layout.element_type_text, type_decls, structured_layouts);
  return element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Scalar &&
         element_layout.scalar_type == bir::TypeKind::I8;
}

std::optional<AggregateByteOffsetProjection> resolve_addressing_child_index_projection(
    std::string_view type_text,
    std::size_t child_index,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  return structured_layouts != nullptr
             ? resolve_aggregate_child_index_projection(
                   type_text, child_index, type_decls, *structured_layouts)
             : resolve_aggregate_child_index_projection(type_text, child_index, type_decls);
}

}  // namespace

bool can_reinterpret_byte_storage_as_type(
    std::string_view storage_type_text,
    std::size_t target_byte_offset,
    std::string_view target_type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls) {
  return can_reinterpret_byte_storage_as_type_impl(
      storage_type_text, target_byte_offset, target_type_text, type_decls, nullptr);
}

bool can_reinterpret_byte_storage_as_type(
    std::string_view storage_type_text,
    std::size_t target_byte_offset,
    std::string_view target_type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return can_reinterpret_byte_storage_as_type_impl(
      storage_type_text, target_byte_offset, target_type_text, type_decls, &structured_layouts);
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
BirFunctionLowerer::find_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls) {
  return find_repeated_aggregate_extent_at_offset_impl(
      type_text, target_offset, repeated_type_text, type_decls, nullptr, true);
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
BirFunctionLowerer::find_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return find_repeated_aggregate_extent_at_offset_impl(
      type_text, target_offset, repeated_type_text, type_decls, &structured_layouts, true);
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
BirFunctionLowerer::find_nested_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls) {
  return find_repeated_aggregate_extent_at_offset_impl(
      type_text, target_offset, repeated_type_text, type_decls, nullptr, false);
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
BirFunctionLowerer::find_nested_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return find_repeated_aggregate_extent_at_offset_impl(
      type_text, target_offset, repeated_type_text, type_decls, &structured_layouts, false);
}

std::optional<BirFunctionLowerer::LocalAggregateGepTarget>
BirFunctionLowerer::resolve_relative_gep_target(
    std::string_view type_text,
    std::int64_t base_byte_offset,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  return resolve_relative_gep_target(
      type_text, base_byte_offset, gep, value_aliases, type_decls, nullptr);
}

std::optional<BirFunctionLowerer::LocalAggregateGepTarget>
BirFunctionLowerer::resolve_relative_gep_target(
    std::string_view type_text,
    std::int64_t base_byte_offset,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(type_text));
  const auto gep_element_type =
      c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str());
  std::int64_t byte_offset = base_byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value()) {
      return std::nullopt;
    }

    const auto layout = lookup_addressing_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
      if (index_pos == 0 && gep_element_type != std::string_view(current_type)) {
        const auto element_layout =
            lookup_addressing_layout(gep_element_type, type_decls, structured_layouts);
        if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }
        byte_offset +=
            static_cast<std::int64_t>(element_layout.size_bytes) * *index_value;
        current_type = gep_element_type;
        continue;
      }
      if (index_pos + 1 != gep.indices.size()) {
        return std::nullopt;
      }
      const auto signed_delta = static_cast<std::int64_t>(layout.size_bytes) * *index_value;
      byte_offset += signed_delta;
      continue;
    }

    if (*index_value < 0) {
      return std::nullopt;
    }

    if (index_pos == 0) {
      if (gep_element_type != std::string_view(current_type) && *index_value == 0 &&
          can_reinterpret_byte_storage_as_type_impl(
              current_type, 0, gep_element_type, type_decls, structured_layouts)) {
        current_type = gep_element_type;
        continue;
      }
      byte_offset +=
          static_cast<std::int64_t>(static_cast<std::size_t>(*index_value) * layout.size_bytes);
      continue;
    }

    const auto projection = resolve_addressing_child_index_projection(
        current_type, static_cast<std::size_t>(*index_value), type_decls, structured_layouts);
    if (!projection.has_value()) {
      return std::nullopt;
    }
    switch (projection->kind) {
      case AggregateByteOffsetProjection::Kind::ArrayElement:
      case AggregateByteOffsetProjection::Kind::StructField:
        byte_offset += static_cast<std::int64_t>(projection->child_start_byte_offset);
        current_type = projection->child_type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  return LocalAggregateGepTarget{
      .type_text = std::string(current_type),
      .byte_offset = byte_offset,
  };
}

std::optional<std::size_t> BirFunctionLowerer::find_pointer_array_length_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const TypeDeclMap& type_decls) {
  const auto projection =
      resolve_aggregate_byte_offset_projection(type_text, target_offset, type_decls);
  if (!projection.has_value()) {
    return std::nullopt;
  }

  switch (projection->kind) {
    case AggregateByteOffsetProjection::Kind::ArrayElement:
      if (projection->target_byte_offset == 0 &&
          projection->child_layout.kind == AggregateTypeLayout::Kind::Scalar &&
          projection->child_layout.scalar_type == bir::TypeKind::Ptr) {
        return projection->layout.array_count;
      }
      return find_pointer_array_length_at_offset(
          projection->child_type_text, projection->byte_offset_within_child, type_decls);
    case AggregateByteOffsetProjection::Kind::StructField:
      return find_pointer_array_length_at_offset(
          projection->child_type_text, projection->byte_offset_within_child, type_decls);
    default:
      return std::nullopt;
  }
}

namespace {

std::optional<std::size_t> find_pointer_array_length_at_offset_impl(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  const auto projection =
      resolve_aggregate_byte_offset_projection(type_text, target_offset, type_decls, structured_layouts);
  if (!projection.has_value()) {
    return std::nullopt;
  }

  switch (projection->kind) {
    case AggregateByteOffsetProjection::Kind::ArrayElement:
      if (projection->target_byte_offset == 0 &&
          projection->child_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Scalar &&
          projection->child_layout.scalar_type == bir::TypeKind::Ptr) {
        return projection->layout.array_count;
      }
      return find_pointer_array_length_at_offset_impl(
          projection->child_type_text,
          projection->byte_offset_within_child,
          type_decls,
          structured_layouts);
    case AggregateByteOffsetProjection::Kind::StructField:
      return find_pointer_array_length_at_offset_impl(
          projection->child_type_text,
          projection->byte_offset_within_child,
          type_decls,
          structured_layouts);
    default:
      return std::nullopt;
  }
}

}  // namespace

std::optional<GlobalAddress> BirFunctionLowerer::resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  return resolve_global_gep_address(
      global_name, type_text, gep, value_aliases, type_decls, nullptr);
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return resolve_global_gep_address(
      global_name, type_text, gep, value_aliases, type_decls, &structured_layouts);
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(type_text));
  std::size_t byte_offset = 0;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = lookup_addressing_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0) {
      if (*index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    const auto projection = resolve_addressing_child_index_projection(
        current_type, static_cast<std::size_t>(*index_value), type_decls, structured_layouts);
    if (!projection.has_value()) {
      return std::nullopt;
    }
    switch (projection->kind) {
      case AggregateByteOffsetProjection::Kind::ArrayElement:
      case AggregateByteOffsetProjection::Kind::StructField:
        byte_offset += projection->child_start_byte_offset;
        current_type = projection->child_type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = lookup_addressing_layout(current_type, type_decls, structured_layouts);
  return GlobalAddress{
      .global_name = std::string(global_name),
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = byte_offset,
  };
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_relative_global_gep_address(
    const GlobalAddress& base_address,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  return resolve_relative_global_gep_address(
      base_address, type_text, gep, value_aliases, type_decls, nullptr);
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_relative_global_gep_address(
    const GlobalAddress& base_address,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return resolve_relative_global_gep_address(
      base_address, type_text, gep, value_aliases, type_decls, &structured_layouts);
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_relative_global_gep_address(
    const GlobalAddress& base_address,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(type_text));
  std::size_t byte_offset = base_address.byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = lookup_addressing_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
      if (index_pos + 1 != gep.indices.size()) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    if (index_pos == 0) {
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    const auto projection = resolve_addressing_child_index_projection(
        current_type, static_cast<std::size_t>(*index_value), type_decls, structured_layouts);
    if (!projection.has_value()) {
      return std::nullopt;
    }
    switch (projection->kind) {
      case AggregateByteOffsetProjection::Kind::ArrayElement:
      case AggregateByteOffsetProjection::Kind::StructField:
        byte_offset += projection->child_start_byte_offset;
        current_type = projection->child_type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = lookup_addressing_layout(current_type, type_decls, structured_layouts);
  return GlobalAddress{
      .global_name = base_address.global_name,
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = byte_offset,
  };
}

std::optional<DynamicGlobalPointerArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_pointer_array_access(
    std::string_view global_name,
    std::string_view base_type_text,
    std::size_t initial_byte_offset,
    bool relative_base,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  return resolve_global_dynamic_pointer_array_access(
      global_name, base_type_text, initial_byte_offset, relative_base, gep,
      value_aliases, type_decls, nullptr);
}

std::optional<DynamicGlobalPointerArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_pointer_array_access(
    std::string_view global_name,
    std::string_view base_type_text,
    std::size_t initial_byte_offset,
    bool relative_base,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return resolve_global_dynamic_pointer_array_access(
      global_name, base_type_text, initial_byte_offset, relative_base, gep,
      value_aliases, type_decls, &structured_layouts);
}

std::optional<DynamicGlobalPointerArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_pointer_array_access(
    std::string_view global_name,
    std::string_view base_type_text,
    std::size_t initial_byte_offset,
    bool relative_base,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(base_type_text));
  std::size_t byte_offset = initial_byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = lookup_addressing_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0 && !relative_base) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    if (index_pos == 0 && relative_base) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value < 0) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        index_value.has_value()) {
      if (*index_value < 0) {
        return std::nullopt;
      }
      const auto projection = resolve_addressing_child_index_projection(
          current_type, static_cast<std::size_t>(*index_value), type_decls, structured_layouts);
      if (!projection.has_value()) {
        return std::nullopt;
      }
      switch (projection->kind) {
        case AggregateByteOffsetProjection::Kind::ArrayElement:
          if (projection->child_layout.size_bytes == 0) {
            return std::nullopt;
          }
          [[fallthrough]];
        case AggregateByteOffsetProjection::Kind::StructField:
          byte_offset += projection->child_start_byte_offset;
          current_type = projection->child_type_text;
          break;
        default:
          return std::nullopt;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            lookup_addressing_layout(layout.element_type_text, type_decls, structured_layouts);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }
        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value() || index_pos + 1 != gep.indices.size() ||
            element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            element_layout.scalar_type != bir::TypeKind::Ptr) {
          return std::nullopt;
        }
        return DynamicGlobalPointerArrayAccess{
            .global_name = std::string(global_name),
            .byte_offset = byte_offset,
            .element_count = layout.array_count,
            .index = *lowered_index,
        };
      }
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<DynamicGlobalAggregateArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_aggregate_array_access(
    const GlobalAddress& base_address,
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const GlobalTypes& global_types,
    const TypeDeclMap& type_decls) {
  return resolve_global_dynamic_aggregate_array_access(
      base_address, base_type_text, gep, value_aliases, global_types, type_decls, nullptr);
}

std::optional<DynamicGlobalAggregateArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_aggregate_array_access(
    const GlobalAddress& base_address,
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const GlobalTypes& global_types,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return resolve_global_dynamic_aggregate_array_access(
      base_address, base_type_text, gep, value_aliases, global_types, type_decls,
      &structured_layouts);
}

std::optional<DynamicGlobalAggregateArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_aggregate_array_access(
    const GlobalAddress& base_address,
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const GlobalTypes& global_types,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto base_layout = lookup_addressing_layout(
      c4c::codegen::lir::trim_lir_arg_text(base_type_text), type_decls, structured_layouts);
  if (base_layout.kind != AggregateTypeLayout::Kind::Struct &&
      base_layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }

  const auto parsed_index = parse_typed_operand(gep.indices.front());
  if (!parsed_index.has_value() ||
      resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
    return std::nullopt;
  }

  const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
  if (!lowered_index.has_value()) {
    return std::nullopt;
  }

  const auto global_it = global_types.find(base_address.global_name);
  if (global_it == global_types.end()) {
    return std::nullopt;
  }

  const auto extent = find_repeated_aggregate_extent_at_offset_impl(
      global_it->second.type_text,
      base_address.byte_offset,
      base_type_text,
      type_decls,
      structured_layouts,
      true);
  if (!extent.has_value()) {
    return std::nullopt;
  }

  return DynamicGlobalAggregateArrayAccess{
      .global_name = base_address.global_name,
      .element_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(base_type_text)),
      .byte_offset = base_address.byte_offset,
      .element_count = extent->element_count,
      .element_stride_bytes = extent->element_stride_bytes,
      .index = *lowered_index,
  };
}

bool BirFunctionLowerer::lower_memory_gep_inst(
    const c4c::codegen::lir::LirGepOp& gep,
    std::vector<bir::Inst>* lowered_insts) {
  auto& value_aliases = value_aliases_;
  auto& local_slot_types = local_slot_types_;
  auto& local_pointer_slots = local_pointer_slots_;
  auto& local_array_slots = local_array_slots_;
  auto& local_pointer_array_bases = local_pointer_array_bases_;
  auto& dynamic_local_pointer_arrays = dynamic_local_pointer_arrays_;
  auto& dynamic_local_aggregate_arrays = dynamic_local_aggregate_arrays_;
  auto& dynamic_pointer_value_arrays = dynamic_pointer_value_arrays_;
  auto& local_aggregate_slots = local_aggregate_slots_;
  auto& local_slot_pointer_values = local_slot_pointer_values_;
  auto& pointer_value_addresses = pointer_value_addresses_;
  auto& global_pointer_slots = global_pointer_slots_;
  auto& dynamic_global_pointer_arrays = dynamic_global_pointer_arrays_;
  auto& dynamic_global_aggregate_arrays = dynamic_global_aggregate_arrays_;
  auto& dynamic_global_scalar_arrays = dynamic_global_scalar_arrays_;
  const auto& aggregate_params = aggregate_params_;
  const auto& loaded_local_scalar_immediates = loaded_local_scalar_immediates_;
  const auto& global_types = global_types_;
  const auto& type_decls = type_decls_;
  const auto fail_gep = [&]() {
    note_function_lowering_family_failure("gep local-memory semantic family");
    return false;
  };
  const auto publish_exact_local_pointer_owner =
      [&](std::string_view result_name, std::string_view slot_name) {
        value_aliases[std::string(result_name)] =
            bir::Value::named(bir::TypeKind::Ptr, std::string(slot_name));
      };
  if (gep.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      (gep.ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
       gep.ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
    return fail_gep();
  }

  ValueMap gep_index_value_aliases = value_aliases;
  for (const auto& [name, immediate] : loaded_local_scalar_immediates) {
    gep_index_value_aliases.try_emplace(name, immediate);
  }

  std::string resolved_slot;
  if (gep.ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = gep.ptr.str().substr(1);
    const auto global_it = global_types.find(global_name);
    if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
      return fail_gep();
    }
    const auto resolved_address = resolve_global_gep_address(
        global_name,
        global_it->second.type_text,
        gep,
        gep_index_value_aliases,
        type_decls,
        structured_layouts_);
    if (resolved_address.has_value()) {
      global_pointer_slots[gep.result.str()] = *resolved_address;
      return true;
    }
    const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
        global_name,
        global_it->second.type_text,
        0,
        false,
        gep,
        gep_index_value_aliases,
        type_decls,
        structured_layouts_);
    if (!dynamic_array.has_value()) {
      return fail_gep();
    }
    dynamic_global_pointer_arrays[gep.result.str()] = std::move(*dynamic_array);
    return true;
  }

  if (const auto aggregate_it = local_aggregate_slots.find(gep.ptr.str());
      aggregate_it != local_aggregate_slots.end()) {
    bool established_aggregate_subobject = false;
    const auto resolved_target = resolve_local_aggregate_gep_target(
        aggregate_it->second.type_text,
        gep,
        value_aliases,
        type_decls,
        &structured_layouts_,
        aggregate_it->second);
    if (resolved_target.has_value()) {
      const auto target_layout =
          lookup_backend_aggregate_type_layout(resolved_target->type_text,
                                               type_decls,
                                               structured_layouts_);
      if (resolved_target->byte_offset >= 0 &&
          (target_layout.kind == AggregateTypeLayout::Kind::Struct ||
           target_layout.kind == AggregateTypeLayout::Kind::Array)) {
        LocalAggregateSlots subobject_slots{
            .storage_type_text = aggregate_it->second.storage_type_text,
            .type_text = resolved_target->type_text,
            .base_byte_offset = static_cast<std::size_t>(resolved_target->byte_offset),
            .leaf_slots = aggregate_it->second.leaf_slots,
        };
        std::optional<std::vector<std::string>> scalar_array_slots;
        if (target_layout.kind == AggregateTypeLayout::Kind::Array) {
          scalar_array_slots = collect_local_scalar_array_slots(
              subobject_slots.type_text, type_decls, structured_layouts_, subobject_slots);
        }
        const auto leaf_it = aggregate_it->second.leaf_slots.find(
            static_cast<std::size_t>(resolved_target->byte_offset));
        if (scalar_array_slots.has_value() && leaf_it != aggregate_it->second.leaf_slots.end()) {
          local_pointer_array_bases[gep.result.str()] = LocalPointerArrayBase{
              .element_slots = *scalar_array_slots,
              .base_index = 0,
          };
          auto subobject_address = LocalSlotAddress{
              .slot_name = leaf_it->second,
              .value_type = bir::TypeKind::Void,
              .byte_offset = 0,
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = subobject_slots.type_text,
          };
          if (const auto array_base_it = local_pointer_array_bases.find(gep.result.str());
              array_base_it != local_pointer_array_bases.end()) {
            subobject_address.array_element_slots = array_base_it->second.element_slots;
            subobject_address.array_base_index = array_base_it->second.base_index;
          }
          local_slot_pointer_values[gep.result.str()] = std::move(subobject_address);
          publish_exact_local_pointer_owner(gep.result.str(), leaf_it->second);
          return true;
        }
        local_aggregate_slots[gep.result.str()] = std::move(subobject_slots);
        if (leaf_it != aggregate_it->second.leaf_slots.end()) {
          auto subobject_address = LocalSlotAddress{
              .slot_name = leaf_it->second,
              .value_type = bir::TypeKind::Void,
              .byte_offset = 0,
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = local_aggregate_slots[gep.result.str()].type_text,
          };
          if (scalar_array_slots.has_value()) {
            local_pointer_array_bases[gep.result.str()] = LocalPointerArrayBase{
                .element_slots = *scalar_array_slots,
                .base_index = 0,
            };
          }
          if (const auto array_base_it = local_pointer_array_bases.find(gep.result.str());
              array_base_it != local_pointer_array_bases.end()) {
            subobject_address.array_element_slots = array_base_it->second.element_slots;
            subobject_address.array_base_index = array_base_it->second.base_index;
          }
          local_slot_pointer_values[gep.result.str()] = std::move(subobject_address);
          publish_exact_local_pointer_owner(gep.result.str(), leaf_it->second);
        }
        established_aggregate_subobject = true;
      }
    }
    const auto pointer_array_slots = resolve_local_aggregate_pointer_array_slots(
        aggregate_it->second.type_text,
        gep,
        value_aliases,
        type_decls,
        &structured_layouts_,
        aggregate_it->second);
    if (pointer_array_slots.has_value()) {
      const auto resolved_slot = resolve_local_aggregate_gep_slot(
          aggregate_it->second.type_text,
          gep,
          value_aliases,
          type_decls,
          &structured_layouts_,
          aggregate_it->second);
      if (resolved_slot.has_value()) {
        local_pointer_slots[gep.result.str()] = *resolved_slot;
        publish_exact_local_pointer_owner(gep.result.str(), *resolved_slot);
      }
      local_pointer_array_bases[gep.result.str()] = LocalPointerArrayBase{
          .element_slots = std::move(*pointer_array_slots),
          .base_index = 0,
      };
      return true;
    }
    const auto resolved_slot = resolve_local_aggregate_gep_slot(
        aggregate_it->second.type_text,
        gep,
        value_aliases,
        type_decls,
        &structured_layouts_,
        aggregate_it->second);
    if (resolved_slot.has_value()) {
      const auto scalar_array_slots = collect_local_scalar_array_slots(
          aggregate_it->second.type_text, type_decls, structured_layouts_, aggregate_it->second);
      if (resolved_target.has_value()) {
        const auto target_layout =
            lookup_backend_aggregate_type_layout(resolved_target->type_text,
                                                 type_decls,
                                                 structured_layouts_);
        const auto slot_type_it = local_slot_types.find(*resolved_slot);
        if (target_layout.kind == AggregateTypeLayout::Kind::Scalar &&
            slot_type_it != local_slot_types.end() &&
            slot_type_it->second != target_layout.scalar_type) {
          local_slot_pointer_values[gep.result.str()] = LocalSlotAddress{
              .slot_name = *resolved_slot,
              .value_type = target_layout.scalar_type,
              .byte_offset = 0,
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = resolved_target->type_text,
          };
          publish_exact_local_pointer_owner(gep.result.str(), *resolved_slot);
          return true;
        }
        if (scalar_array_slots.has_value() && !scalar_array_slots->empty()) {
          const auto array_layout =
              lookup_backend_aggregate_type_layout(aggregate_it->second.type_text,
                                                   type_decls,
                                                   structured_layouts_);
          const auto element_layout =
              lookup_backend_aggregate_type_layout(array_layout.element_type_text,
                                                   type_decls,
                                                   structured_layouts_);
          const auto relative_byte_offset =
              resolved_target->byte_offset -
              static_cast<std::int64_t>(aggregate_it->second.base_byte_offset);
          if (array_layout.kind == AggregateTypeLayout::Kind::Array &&
              element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
              element_layout.size_bytes != 0 && relative_byte_offset >= 0 &&
              relative_byte_offset % static_cast<std::int64_t>(element_layout.size_bytes) == 0) {
            const auto base_index = static_cast<std::size_t>(
                relative_byte_offset / static_cast<std::int64_t>(element_layout.size_bytes));
            if (base_index < scalar_array_slots->size()) {
              local_pointer_array_bases[gep.result.str()] = LocalPointerArrayBase{
                  .element_slots = *scalar_array_slots,
                  .base_index = base_index,
              };
            }
          }
        }
      }
      local_pointer_slots[gep.result.str()] = *resolved_slot;
      publish_exact_local_pointer_owner(gep.result.str(), *resolved_slot);
      return true;
    }
    if (established_aggregate_subobject) {
      return true;
    }
    const auto dynamic_array = resolve_local_aggregate_dynamic_pointer_array_access(
        aggregate_it->second.type_text,
        gep,
        value_aliases,
        type_decls,
        &structured_layouts_,
        aggregate_it->second);
    if (dynamic_array.has_value()) {
      dynamic_local_pointer_arrays[gep.result.str()] = std::move(*dynamic_array);
      return true;
    }
    const auto dynamic_aggregate = resolve_local_dynamic_aggregate_array_access(
        aggregate_it->second.type_text,
        gep,
        value_aliases,
        type_decls,
        &structured_layouts_,
        aggregate_it->second);
    if (!dynamic_aggregate.has_value()) {
      return fail_gep();
    }
    const auto element_layout =
        lookup_backend_aggregate_type_layout(dynamic_aggregate->element_type_text,
                                             type_decls,
                                             structured_layouts_);
    if (element_layout.kind == AggregateTypeLayout::Kind::Struct ||
        element_layout.kind == AggregateTypeLayout::Kind::Array) {
      std::vector<bir::Value> element_values;
      element_values.reserve(dynamic_aggregate->element_count);
      for (std::size_t element_index = 0;
           element_index < dynamic_aggregate->element_count;
           ++element_index) {
        const std::string element_name =
            gep.result.str() + ".elt" + std::to_string(element_index);
        local_aggregate_slots[element_name] = LocalAggregateSlots{
            .storage_type_text = aggregate_it->second.storage_type_text,
            .type_text = dynamic_aggregate->element_type_text,
            .base_byte_offset =
                dynamic_aggregate->byte_offset +
                element_index * dynamic_aggregate->element_stride_bytes,
            .leaf_slots = aggregate_it->second.leaf_slots,
        };
        element_values.push_back(bir::Value::named(bir::TypeKind::Ptr, element_name));
      }
      const auto selected_value = synthesize_pointer_array_selects(
          gep.result.str(), element_values, dynamic_aggregate->index, lowered_insts);
      if (!selected_value.has_value()) {
        return fail_gep();
      }
      if (selected_value->kind != bir::Value::Kind::Named ||
          selected_value->name != gep.result.str()) {
        value_aliases[gep.result.str()] = *selected_value;
      }
    }
    dynamic_local_aggregate_arrays[gep.result.str()] = std::move(*dynamic_aggregate);
    return true;
  }

  if (const auto handled = try_lower_local_array_slot_gep(gep,
                                                          value_aliases,
                                                          local_array_slots,
                                                          &local_pointer_slots,
                                                          &local_pointer_array_bases,
                                                          &dynamic_local_pointer_arrays);
      handled.has_value()) {
    if (!*handled) {
      return fail_gep();
    }
    if (local_pointer_slots.find(gep.result.str()) == local_pointer_slots.end()) {
      return true;
    }
    resolved_slot = local_pointer_slots[gep.result.str()];
  } else if (const auto handled = try_lower_local_pointer_array_base_gep(gep,
                                                                         value_aliases,
                                                                         type_decls,
                                                                         &structured_layouts_,
                                                                         local_slot_types,
                                                                         &local_pointer_slots,
                                                                         &local_pointer_array_bases,
                                                                         &dynamic_local_pointer_arrays,
                                                                         &dynamic_local_aggregate_arrays,
                                                                         &local_slot_pointer_values);
             handled.has_value()) {
    if (!*handled) {
      return fail_gep();
    }
    if (local_pointer_slots.find(gep.result.str()) == local_pointer_slots.end()) {
      return true;
    }
    resolved_slot = local_pointer_slots[gep.result.str()];
  } else if (const auto global_ptr_it = global_pointer_slots.find(gep.ptr.str());
             global_ptr_it != global_pointer_slots.end()) {
    auto base_address = global_ptr_it->second;
    if (const auto honest_base = resolve_honest_pointer_base(base_address, global_types);
        honest_base.has_value()) {
      base_address = *honest_base;
    }
    const auto resolved_address = resolve_relative_global_gep_address(
        base_address,
        gep.element_type.str(),
        gep,
        gep_index_value_aliases,
        type_decls,
        structured_layouts_);
    if (resolved_address.has_value()) {
      global_pointer_slots[gep.result.str()] = *resolved_address;
      return true;
    }
    const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
        base_address.global_name,
        gep.element_type.str(),
        base_address.byte_offset,
        true,
        gep,
        gep_index_value_aliases,
        type_decls,
        structured_layouts_);
    if (!dynamic_array.has_value()) {
      const auto dynamic_aggregate = resolve_global_dynamic_aggregate_array_access(
          base_address,
          gep.element_type.str(),
          gep,
          gep_index_value_aliases,
          global_types,
          type_decls,
          structured_layouts_);
      if (dynamic_aggregate.has_value()) {
        dynamic_global_aggregate_arrays[gep.result.str()] = std::move(*dynamic_aggregate);
        return true;
      }
      if (gep.indices.size() != 1 || base_address.value_type != bir::TypeKind::Ptr) {
        return fail_gep();
      }
      const auto parsed_index = parse_typed_operand(gep.indices.front());
      if (!parsed_index.has_value()) {
        return fail_gep();
      }
      const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
      if (!index_value.has_value()) {
        return fail_gep();
      }
      const auto global_it = global_types.find(base_address.global_name);
      if (global_it == global_types.end()) {
        return fail_gep();
      }
      const auto array_length = find_pointer_array_length_at_offset_impl(
          global_it->second.type_text,
          base_address.byte_offset,
          type_decls,
          structured_layouts_);
      if (!array_length.has_value()) {
        return fail_gep();
      }
      dynamic_global_pointer_arrays[gep.result.str()] = DynamicGlobalPointerArrayAccess{
          .global_name = base_address.global_name,
          .byte_offset = base_address.byte_offset,
          .element_count = *array_length,
          .index = *index_value,
      };
      return true;
    }
    dynamic_global_pointer_arrays[gep.result.str()] = std::move(*dynamic_array);
    return true;
  } else if (const auto global_aggregate_it = dynamic_global_aggregate_arrays.find(gep.ptr.str());
             global_aggregate_it != dynamic_global_aggregate_arrays.end()) {
    if (const auto aggregate_target = resolve_relative_gep_target(
            global_aggregate_it->second.element_type_text,
            0,
            gep,
            value_aliases,
            type_decls,
            &structured_layouts_);
        aggregate_target.has_value() && aggregate_target->byte_offset >= 0) {
      const auto target_layout =
          lookup_backend_aggregate_type_layout(aggregate_target->type_text,
                                               type_decls,
                                               structured_layouts_);
      if (target_layout.kind == AggregateTypeLayout::Kind::Struct ||
          target_layout.kind == AggregateTypeLayout::Kind::Array) {
        dynamic_global_aggregate_arrays[gep.result.str()] = DynamicGlobalAggregateArrayAccess{
            .global_name = global_aggregate_it->second.global_name,
            .element_type_text = std::move(aggregate_target->type_text),
            .byte_offset =
                global_aggregate_it->second.byte_offset +
                static_cast<std::size_t>(aggregate_target->byte_offset),
            .element_count = global_aggregate_it->second.element_count,
            .element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
            .index = global_aggregate_it->second.index,
        };
        return true;
      }
      if (target_layout.kind == AggregateTypeLayout::Kind::Scalar &&
          target_layout.scalar_type != bir::TypeKind::Void) {
        std::size_t element_count = 1;
        std::size_t element_stride_bytes = 0;
        if (const auto extent = find_repeated_aggregate_extent_at_offset_impl(
                global_aggregate_it->second.element_type_text,
                static_cast<std::size_t>(aggregate_target->byte_offset),
                aggregate_target->type_text,
                type_decls,
                &structured_layouts_,
                true);
            extent.has_value()) {
          element_count = extent->element_count;
          element_stride_bytes = extent->element_stride_bytes;
        }
        const auto zero_index = make_index_immediate(bir::TypeKind::I64, 0);
        if (!zero_index.has_value()) {
          return fail_gep();
        }
        dynamic_global_scalar_arrays[gep.result.str()] = DynamicGlobalScalarArrayAccess{
            .global_name = global_aggregate_it->second.global_name,
            .element_type = target_layout.scalar_type,
            .byte_offset =
                global_aggregate_it->second.byte_offset +
                static_cast<std::size_t>(aggregate_target->byte_offset),
            .outer_element_count = global_aggregate_it->second.element_count,
            .outer_element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
            .outer_index = global_aggregate_it->second.index,
            .element_count = element_count,
            .element_stride_bytes = element_stride_bytes,
            .index = *zero_index,
        };
        return true;
      }
    }
    const auto element_leaf = resolve_relative_global_gep_address(
        GlobalAddress{},
        global_aggregate_it->second.element_type_text,
        gep,
        value_aliases,
        type_decls,
        structured_layouts_);
    if (!element_leaf.has_value() || element_leaf->value_type != bir::TypeKind::Ptr) {
      return fail_gep();
    }
    dynamic_global_pointer_arrays[gep.result.str()] = DynamicGlobalPointerArrayAccess{
        .global_name = global_aggregate_it->second.global_name,
        .byte_offset = global_aggregate_it->second.byte_offset + element_leaf->byte_offset,
        .element_count = global_aggregate_it->second.element_count,
        .element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
        .index = global_aggregate_it->second.index,
    };
    return true;
  } else if (const auto global_scalar_it = dynamic_global_scalar_arrays.find(gep.ptr.str());
             global_scalar_it != dynamic_global_scalar_arrays.end()) {
    if (gep.indices.empty() || gep.indices.size() > 2 ||
        global_scalar_it->second.element_count == 0 ||
        global_scalar_it->second.element_stride_bytes == 0) {
      return fail_gep();
    }
    std::size_t index_pos = 0;
    if (gep.indices.size() == 2) {
      const auto base_index = parse_typed_operand(gep.indices.front());
      if (!base_index.has_value()) {
        return fail_gep();
      }
      const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
      if (!base_imm.has_value() || *base_imm != 0) {
        return fail_gep();
      }
      index_pos = 1;
    }
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return fail_gep();
    }
    const auto zero_index = make_index_immediate(bir::TypeKind::I64, 0);
    if (!zero_index.has_value()) {
      return fail_gep();
    }
    if (const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
        index_imm.has_value()) {
      if (*index_imm < 0 ||
          static_cast<std::size_t>(*index_imm) >= global_scalar_it->second.element_count) {
        return fail_gep();
      }
      dynamic_global_scalar_arrays[gep.result.str()] = DynamicGlobalScalarArrayAccess{
          .global_name = global_scalar_it->second.global_name,
          .element_type = global_scalar_it->second.element_type,
          .byte_offset =
              global_scalar_it->second.byte_offset +
              static_cast<std::size_t>(*index_imm) *
                  global_scalar_it->second.element_stride_bytes,
          .outer_element_count = global_scalar_it->second.outer_element_count,
          .outer_element_stride_bytes = global_scalar_it->second.outer_element_stride_bytes,
          .outer_index = global_scalar_it->second.outer_index,
          .element_count =
              global_scalar_it->second.element_count - static_cast<std::size_t>(*index_imm),
          .element_stride_bytes = global_scalar_it->second.element_stride_bytes,
          .index = *zero_index,
      };
      return true;
    }
    const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
    if (!index_value.has_value()) {
      return fail_gep();
    }
    dynamic_global_scalar_arrays[gep.result.str()] = DynamicGlobalScalarArrayAccess{
        .global_name = global_scalar_it->second.global_name,
        .element_type = global_scalar_it->second.element_type,
        .byte_offset = global_scalar_it->second.byte_offset,
        .outer_element_count = global_scalar_it->second.outer_element_count,
        .outer_element_stride_bytes = global_scalar_it->second.outer_element_stride_bytes,
        .outer_index = global_scalar_it->second.outer_index,
        .element_count = global_scalar_it->second.element_count,
        .element_stride_bytes = global_scalar_it->second.element_stride_bytes,
        .index = *index_value,
    };
    return true;
  } else if (const auto handled_dynamic_local_aggregate_gep =
                 try_lower_dynamic_local_aggregate_gep_projection(gep,
                                                                  dynamic_local_aggregate_arrays,
                                                                  value_aliases,
                                                                  type_decls,
                                                                  &structured_layouts_,
                                                                  &dynamic_local_pointer_arrays);
             handled_dynamic_local_aggregate_gep.has_value()) {
    if (!*handled_dynamic_local_aggregate_gep) {
      return fail_gep();
    }
    return true;
  } else if (const auto handled_local_slot_gep =
                 try_lower_local_slot_pointer_gep(
                     gep,
                     value_aliases,
                     type_decls,
                     &structured_layouts_,
                     local_slot_types,
                     &local_slot_pointer_values);
             handled_local_slot_gep.has_value()) {
    if (!*handled_local_slot_gep) {
      return fail_gep();
    }
    return true;
  } else {
    const auto addressed_ptr_it = pointer_value_addresses.find(gep.ptr.str());
    const auto ptr_it = local_pointer_slots.find(gep.ptr.str());
    if (addressed_ptr_it != pointer_value_addresses.end() || ptr_it == local_pointer_slots.end()) {
      const auto base_pointer = addressed_ptr_it != pointer_value_addresses.end()
                                    ? std::optional<bir::Value>(addressed_ptr_it->second.base_value)
                                    : lower_value(gep.ptr, bir::TypeKind::Ptr, value_aliases);
      if (base_pointer.has_value()) {
        const auto base_byte_offset = addressed_ptr_it != pointer_value_addresses.end()
                                          ? addressed_ptr_it->second.byte_offset
                                          : 0;
        auto addressed_base_type_text =
            addressed_ptr_it != pointer_value_addresses.end()
                ? addressed_ptr_it->second.type_text
                : std::string{};
        if (addressed_base_type_text.empty() &&
            base_pointer->kind == bir::Value::Kind::Named &&
            base_pointer->type == bir::TypeKind::Ptr) {
          if (const auto aggregate_param_it = aggregate_params.find(base_pointer->name);
              aggregate_param_it != aggregate_params.end()) {
            addressed_base_type_text = aggregate_param_it->second.type_text;
          }
        }
        const auto resolved_target = resolve_relative_gep_target(
            !addressed_base_type_text.empty()
                ? addressed_base_type_text
                : std::string(c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str())),
            base_byte_offset,
            gep,
            value_aliases,
            type_decls,
            &structured_layouts_);
        if (resolved_target.has_value() && resolved_target->byte_offset >= 0) {
          const auto leaf_layout =
              lookup_backend_aggregate_type_layout(resolved_target->type_text,
                                                   type_decls,
                                                   structured_layouts_);
          auto storage_type_text =
              addressed_ptr_it != pointer_value_addresses.end() &&
                      !addressed_ptr_it->second.storage_type_text.empty()
                  ? addressed_ptr_it->second.storage_type_text
                  : std::string(c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str()));
          if (leaf_layout.kind == AggregateTypeLayout::Kind::Struct ||
              leaf_layout.kind == AggregateTypeLayout::Kind::Array) {
            if (resolved_target->byte_offset == static_cast<std::int64_t>(base_byte_offset)) {
              storage_type_text = resolved_target->type_text;
            }
          }
          pointer_value_addresses[gep.result.str()] = PointerAddress{
              .base_value = *base_pointer,
              .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                                ? leaf_layout.scalar_type
                                : bir::TypeKind::Void,
              .byte_offset = static_cast<std::size_t>(resolved_target->byte_offset),
              .storage_type_text = std::move(storage_type_text),
              .type_text = std::move(resolved_target->type_text),
          };
          if (resolved_target->byte_offset == base_byte_offset) {
            value_aliases[gep.result.str()] = *base_pointer;
          }
          return true;
        }

        if (addressed_ptr_it != pointer_value_addresses.end() &&
            (gep.indices.size() == 1 || gep.indices.size() == 2) &&
            ((addressed_ptr_it->second.dynamic_element_count != 0 &&
              addressed_ptr_it->second.dynamic_element_stride_bytes != 0) ||
             (!addressed_ptr_it->second.storage_type_text.empty() &&
              !addressed_ptr_it->second.type_text.empty()))) {
          std::size_t dynamic_index_pos = 0;
          if (gep.indices.size() == 2) {
            const auto base_index = parse_typed_operand(gep.indices.front());
            if (!base_index.has_value()) {
              return fail_gep();
            }
            const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
            if (!base_imm.has_value() || *base_imm != 0) {
              return fail_gep();
            }
            dynamic_index_pos = 1;
          }
          const auto parsed_index = parse_typed_operand(gep.indices[dynamic_index_pos]);
          if (parsed_index.has_value() &&
              !resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
            const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
            if (lowered_index.has_value()) {
              const auto dynamic_element_type =
                  lower_scalar_or_function_pointer_type(gep.element_type.str());
              if (addressed_ptr_it->second.dynamic_element_count != 0 &&
                  addressed_ptr_it->second.dynamic_element_stride_bytes != 0 &&
                  dynamic_element_type.has_value() &&
                  type_size_bytes(*dynamic_element_type) ==
                      addressed_ptr_it->second.dynamic_element_stride_bytes) {
                dynamic_pointer_value_arrays[gep.result.str()] = DynamicPointerValueArrayAccess{
                    .base_value = addressed_ptr_it->second.base_value,
                    .element_type = *dynamic_element_type,
                    .byte_offset = addressed_ptr_it->second.byte_offset,
                    .element_count = addressed_ptr_it->second.dynamic_element_count,
                    .element_stride_bytes =
                        addressed_ptr_it->second.dynamic_element_stride_bytes,
                    .index = *lowered_index,
                };
                return true;
              }
              const auto element_layout = lookup_backend_aggregate_type_layout(
                  addressed_ptr_it->second.type_text, type_decls, structured_layouts_);
              if (element_layout.kind == AggregateTypeLayout::Kind::Array) {
                const auto array_element_layout = lookup_backend_aggregate_type_layout(
                    element_layout.element_type_text, type_decls, structured_layouts_);
                if (array_element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
                    array_element_layout.scalar_type != bir::TypeKind::Void &&
                    array_element_layout.size_bytes != 0) {
                  dynamic_pointer_value_arrays[gep.result.str()] = DynamicPointerValueArrayAccess{
                      .base_value = addressed_ptr_it->second.base_value,
                      .element_type = array_element_layout.scalar_type,
                      .byte_offset = addressed_ptr_it->second.byte_offset,
                      .element_count = element_layout.array_count,
                      .element_stride_bytes = array_element_layout.size_bytes,
                      .index = *lowered_index,
                  };
                  return true;
                }
              }
              const auto extent = find_repeated_aggregate_extent_at_offset_impl(
                  addressed_ptr_it->second.storage_type_text,
                  addressed_ptr_it->second.byte_offset,
                  addressed_ptr_it->second.type_text,
                  type_decls,
                  &structured_layouts_,
                  true);
              if (extent.has_value() &&
                  element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
                  element_layout.scalar_type != bir::TypeKind::Void) {
                dynamic_pointer_value_arrays[gep.result.str()] = DynamicPointerValueArrayAccess{
                    .base_value = addressed_ptr_it->second.base_value,
                    .element_type = element_layout.scalar_type,
                    .byte_offset = addressed_ptr_it->second.byte_offset,
                    .element_count = extent->element_count,
                    .element_stride_bytes = extent->element_stride_bytes,
                    .index = *lowered_index,
                };
                return true;
              }
            }
          }
        }

        std::size_t raw_index_pos = 0;
        if (gep.indices.size() == 2) {
          const auto base_index = parse_typed_operand(gep.indices.front());
          if (!base_index.has_value()) {
            return fail_gep();
          }
          const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
          if (!base_imm.has_value() || *base_imm != 0) {
            return fail_gep();
          }
          raw_index_pos = 1;
        }
        if (addressed_ptr_it == pointer_value_addresses.end() &&
            (gep.indices.size() == 1 || gep.indices.size() == 2) &&
            c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str()) == "i8") {
          const auto parsed_index = parse_typed_operand(gep.indices[raw_index_pos]);
          if (!parsed_index.has_value()) {
            return fail_gep();
          }
          auto raw_offset = lower_typed_index_value(*parsed_index, value_aliases);
          if (!raw_offset.has_value()) {
            return fail_gep();
          }
          if (raw_offset->kind == bir::Value::Kind::Immediate &&
              raw_offset->type != bir::TypeKind::I64) {
            raw_offset = bir::Value::immediate_i64(raw_offset->immediate);
          }
          if (raw_offset->kind == bir::Value::Kind::Named &&
              raw_offset->type != bir::TypeKind::I64) {
            return fail_gep();
          }
          if (raw_offset->kind == bir::Value::Kind::Immediate && raw_offset->immediate == 0) {
            value_aliases[gep.result.str()] = *base_pointer;
            return true;
          }
          lowered_insts->push_back(bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Add,
              .result = bir::Value::named(bir::TypeKind::Ptr, gep.result.str()),
              .operand_type = bir::TypeKind::Ptr,
              .lhs = *base_pointer,
              .rhs = *raw_offset,
          });
          value_aliases[gep.result.str()] = bir::Value::named(bir::TypeKind::Ptr,
                                                               gep.result.str());
          return true;
        }
      }
    }
    const auto handled_local_pointer_slot_base_gep = try_lower_local_pointer_slot_base_gep(
        gep,
        value_aliases,
        type_decls,
        &structured_layouts_,
        local_slot_types,
        local_array_slots,
        local_aggregate_slots,
        &local_pointer_slots,
        &local_pointer_array_bases,
        &dynamic_local_pointer_arrays,
        &dynamic_local_aggregate_arrays);
    if (!handled_local_pointer_slot_base_gep.has_value() || !*handled_local_pointer_slot_base_gep) {
      return fail_gep();
    }
    return true;
  }

  if (!resolved_slot.empty()) {
    local_pointer_slots[gep.result.str()] = std::move(resolved_slot);
    return true;
  }
  return false;
}

}  // namespace c4c::backend
