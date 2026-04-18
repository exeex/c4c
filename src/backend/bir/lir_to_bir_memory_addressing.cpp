#include "lir_to_bir.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

using DynamicGlobalAggregateArrayAccess = BirFunctionLowerer::DynamicGlobalAggregateArrayAccess;
using DynamicGlobalPointerArrayAccess = BirFunctionLowerer::DynamicGlobalPointerArrayAccess;
using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using DynamicLocalPointerArrayAccess = BirFunctionLowerer::DynamicLocalPointerArrayAccess;
using GlobalAddress = BirFunctionLowerer::GlobalAddress;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;

static bool can_reinterpret_byte_storage_view(std::string_view storage_type_text,
                                              std::string_view target_type_text,
                                              const BirFunctionLowerer::TypeDeclMap& type_decls) {
  const auto storage_layout = compute_aggregate_type_layout(storage_type_text, type_decls);
  const auto target_layout = compute_aggregate_type_layout(target_type_text, type_decls);
  if (storage_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Array ||
      target_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      storage_layout.size_bytes == 0 || storage_layout.size_bytes != target_layout.size_bytes) {
    return false;
  }
  const auto element_layout = compute_aggregate_type_layout(storage_layout.element_type_text, type_decls);
  return element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Scalar &&
         element_layout.scalar_type == bir::TypeKind::I8;
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
BirFunctionLowerer::find_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid || target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      if (nested_offset == 0 &&
          c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text) ==
              c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
        return AggregateArrayExtent{
            .element_count = layout.array_count - element_index,
            .element_stride_bytes = element_layout.size_bytes,
        };
      }
      return find_repeated_aggregate_extent_at_offset(
          layout.element_type_text, nested_offset, repeated_type_text, type_decls);
    }
    case AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        return find_repeated_aggregate_extent_at_offset(layout.fields[index].type_text,
                                                        target_offset - field_begin,
                                                        repeated_type_text,
                                                        type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<BirFunctionLowerer::LocalAggregateGepTarget>
BirFunctionLowerer::resolve_relative_gep_target(
    std::string_view type_text,
    std::int64_t base_byte_offset,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
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

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
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
      if (gep_element_type != current_type && *index_value == 0 &&
          can_reinterpret_byte_storage_view(current_type, gep_element_type, type_decls)) {
        current_type = gep_element_type;
        continue;
      }
      byte_offset +=
          static_cast<std::int64_t>(static_cast<std::size_t>(*index_value) * layout.size_bytes);
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
        byte_offset += static_cast<std::int64_t>(
            static_cast<std::size_t>(*index_value) * element_layout.size_bytes);
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::int64_t>(
            layout.fields[static_cast<std::size_t>(*index_value)].byte_offset);
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
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
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid || target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      if (target_offset == 0 && element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
          element_layout.scalar_type == bir::TypeKind::Ptr) {
        return layout.array_count;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      return find_pointer_array_length_at_offset(
          layout.element_type_text, nested_offset, type_decls);
    }
    case AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        return find_pointer_array_length_at_offset(
            layout.fields[index].type_text, target_offset - field_begin, type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
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

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0) {
      if (*index_value != 0) {
        return std::nullopt;
      }
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
        current_type = layout.element_type_text;
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = layout.fields[static_cast<std::size_t>(*index_value)].type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
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
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
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

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
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

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = layout.element_type_text;
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = layout.fields[static_cast<std::size_t>(*index_value)].type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
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
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = initial_byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
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

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
            index_value.has_value()) {
          if (*index_value < 0 || static_cast<std::size_t>(*index_value) >= layout.array_count) {
            return std::nullopt;
          }
          byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
          current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
          break;
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
      case AggregateTypeLayout::Kind::Struct: {
        const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        if (!index_value.has_value() || *index_value < 0 ||
            static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
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
  if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto base_layout = compute_aggregate_type_layout(
      c4c::codegen::lir::trim_lir_arg_text(base_type_text), type_decls);
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

  const auto extent = find_repeated_aggregate_extent_at_offset(
      global_it->second.type_text, base_address.byte_offset, base_type_text, type_decls);
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

std::optional<std::string> BirFunctionLowerer::resolve_local_aggregate_gep_slot(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  const auto gep_element_type = c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str());
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (gep_element_type == current_type && layout.kind == AggregateTypeLayout::Kind::Scalar) {
        if (*index_value != 0) {
          return std::nullopt;
        }
        continue;
      }
      if (gep_element_type == current_type && *index_value != 0) {
        return std::nullopt;
      }
      if (gep_element_type == current_type) {
        continue;
      }
      if (*index_value == 0 &&
          can_reinterpret_byte_storage_view(current_type, gep_element_type, type_decls)) {
        current_type = gep_element_type;
        continue;
      }
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
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  if (leaf_layout.kind != AggregateTypeLayout::Kind::Scalar) {
    return std::nullopt;
  }

  const auto slot_it = aggregate_slots.leaf_slots.find(byte_offset);
  if (slot_it == aggregate_slots.leaf_slots.end()) {
    return std::nullopt;
  }
  return slot_it->second;
}

std::optional<std::vector<std::string>>
BirFunctionLowerer::resolve_local_aggregate_pointer_array_slots(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  const auto gep_element_type = c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str());
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (*index_value == 0 && gep_element_type != current_type &&
          can_reinterpret_byte_storage_view(current_type, gep_element_type, type_decls)) {
        current_type = gep_element_type;
        continue;
      }
      if (*index_value != 0) {
        const auto extent = find_repeated_aggregate_extent_at_offset(
            aggregate_slots.storage_type_text,
            aggregate_slots.base_byte_offset,
            base_type_text,
            type_decls);
        if (!extent.has_value() ||
            static_cast<std::size_t>(*index_value) >= extent->element_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * extent->element_stride_bytes;
      }
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
        if (element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
            element_layout.size_bytes != 0 && *index_value == 0 &&
            &raw_index == &gep.indices.back()) {
          std::vector<std::string> element_slots;
          element_slots.reserve(layout.array_count);
          for (std::size_t index = 0; index < layout.array_count; ++index) {
            const auto slot_it =
                aggregate_slots.leaf_slots.find(byte_offset + index * element_layout.size_bytes);
            if (slot_it == aggregate_slots.leaf_slots.end()) {
              return std::nullopt;
            }
            element_slots.push_back(slot_it->second);
          }
          return element_slots;
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
      default:
        return std::nullopt;
    }
  }

  const auto layout = compute_aggregate_type_layout(current_type, type_decls);
  if (layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
      element_layout.scalar_type != bir::TypeKind::Ptr || element_layout.size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> element_slots;
  element_slots.reserve(layout.array_count);
  for (std::size_t index = 0; index < layout.array_count; ++index) {
    const auto slot_it =
        aggregate_slots.leaf_slots.find(byte_offset + index * element_layout.size_bytes);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    element_slots.push_back(slot_it->second);
  }
  return element_slots;
}

std::optional<DynamicLocalPointerArrayAccess>
BirFunctionLowerer::resolve_local_aggregate_dynamic_pointer_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value != 0) {
        return std::nullopt;
      }
      saw_base_index = true;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
            index_value.has_value()) {
          if (*index_value < 0 || static_cast<std::size_t>(*index_value) >= layout.array_count) {
            return std::nullopt;
          }
          byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
          current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
          break;
        }

        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value()) {
          return std::nullopt;
        }

        std::size_t element_leaf_offset = 0;
        std::string_view element_type =
            c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        for (std::size_t tail_pos = index_pos + 1; tail_pos < gep.indices.size(); ++tail_pos) {
          const auto tail_index = parse_typed_operand(gep.indices[tail_pos]);
          if (!tail_index.has_value()) {
            return std::nullopt;
          }
          const auto tail_value = resolve_index_operand(tail_index->operand, value_aliases);
          if (!tail_value.has_value() || *tail_value < 0) {
            return std::nullopt;
          }

          const auto tail_layout = compute_aggregate_type_layout(element_type, type_decls);
          if (tail_layout.kind == AggregateTypeLayout::Kind::Invalid ||
              tail_layout.size_bytes == 0 || tail_layout.align_bytes == 0) {
            return std::nullopt;
          }

          switch (tail_layout.kind) {
            case AggregateTypeLayout::Kind::Array: {
              const auto nested_layout =
                  compute_aggregate_type_layout(tail_layout.element_type_text, type_decls);
              if (nested_layout.kind == AggregateTypeLayout::Kind::Invalid ||
                  static_cast<std::size_t>(*tail_value) >= tail_layout.array_count) {
                return std::nullopt;
              }
              element_leaf_offset +=
                  static_cast<std::size_t>(*tail_value) * nested_layout.size_bytes;
              element_type =
                  c4c::codegen::lir::trim_lir_arg_text(tail_layout.element_type_text);
              break;
            }
            case AggregateTypeLayout::Kind::Struct:
              if (static_cast<std::size_t>(*tail_value) >= tail_layout.fields.size()) {
                return std::nullopt;
              }
              element_leaf_offset +=
                  tail_layout.fields[static_cast<std::size_t>(*tail_value)].byte_offset;
              element_type = c4c::codegen::lir::trim_lir_arg_text(
                  tail_layout.fields[static_cast<std::size_t>(*tail_value)].type_text);
              break;
            default:
              return std::nullopt;
          }
        }

        const auto final_layout = compute_aggregate_type_layout(element_type, type_decls);
        if (final_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            final_layout.scalar_type != bir::TypeKind::Ptr) {
          return std::nullopt;
        }

        std::vector<std::string> element_slots;
        element_slots.reserve(layout.array_count);
        for (std::size_t element_index = 0; element_index < layout.array_count; ++element_index) {
          const auto slot_it = aggregate_slots.leaf_slots.find(
              byte_offset + element_index * element_layout.size_bytes + element_leaf_offset);
          if (slot_it == aggregate_slots.leaf_slots.end()) {
            return std::nullopt;
          }
          element_slots.push_back(slot_it->second);
        }
        return DynamicLocalPointerArrayAccess{
            .element_slots = std::move(element_slots),
            .index = *lowered_index,
        };
      }
      case AggregateTypeLayout::Kind::Struct: {
        const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        if (!index_value.has_value() || *index_value < 0 ||
            static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      }
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<BirFunctionLowerer::LocalAggregateGepTarget>
BirFunctionLowerer::resolve_local_aggregate_gep_target(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  const auto gep_element_type = c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str());
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (gep_element_type == current_type && *index_value != 0) {
        const auto extent = find_repeated_aggregate_extent_at_offset(
            aggregate_slots.storage_type_text,
            aggregate_slots.base_byte_offset,
            base_type_text,
            type_decls);
        if (!extent.has_value() ||
            static_cast<std::size_t>(*index_value) >= extent->element_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * extent->element_stride_bytes;
        continue;
      }
      if (gep_element_type == current_type) {
        continue;
      }
      if (*index_value == 0 &&
          can_reinterpret_byte_storage_view(current_type, gep_element_type, type_decls)) {
        current_type = gep_element_type;
        continue;
      }
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
      default:
        return std::nullopt;
    }
  }

  return LocalAggregateGepTarget{
      .type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(current_type)),
      .byte_offset = static_cast<std::int64_t>(byte_offset),
  };
}

std::optional<DynamicLocalAggregateArrayAccess>
BirFunctionLowerer::resolve_local_dynamic_aggregate_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::size_t index_pos = 0;
  if (gep.indices.size() == 2) {
    const auto base_index = parse_typed_operand(gep.indices.front());
    if (!base_index.has_value()) {
      return std::nullopt;
    }
    const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
    if (!base_imm.has_value() || *base_imm != 0) {
      return std::nullopt;
    }
    index_pos = 1;
  } else if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
  if (!parsed_index.has_value() ||
      resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
    return std::nullopt;
  }

  const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
  if (!lowered_index.has_value()) {
    return std::nullopt;
  }

  const auto extent = find_repeated_aggregate_extent_at_offset(
      aggregate_slots.storage_type_text,
      aggregate_slots.base_byte_offset,
      base_type_text,
      type_decls);
  if (!extent.has_value()) {
    return std::nullopt;
  }

  return DynamicLocalAggregateArrayAccess{
      .element_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(base_type_text)),
      .byte_offset = aggregate_slots.base_byte_offset,
      .element_count = extent->element_count,
      .element_stride_bytes = extent->element_stride_bytes,
      .leaf_slots = aggregate_slots.leaf_slots,
      .index = *lowered_index,
  };
}

}  // namespace c4c::backend
