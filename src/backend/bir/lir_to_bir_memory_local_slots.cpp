#include "lir_to_bir.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using DynamicLocalPointerArrayAccess = BirFunctionLowerer::DynamicLocalPointerArrayAccess;
using LocalAggregateGepTarget = BirFunctionLowerer::LocalAggregateGepTarget;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;

std::optional<std::vector<std::string>> BirFunctionLowerer::collect_local_scalar_array_slots(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  const auto layout = compute_aggregate_type_layout(
      c4c::codegen::lir::trim_lir_arg_text(type_text), type_decls);
  if (layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar || element_layout.size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> element_slots;
  std::size_t slot_count = layout.array_count;
  struct RepeatedAggregateExtent {
    std::size_t element_count = 0;
    std::size_t element_stride_bytes = 0;
  };
  const auto find_repeated_extent =
      [&](const auto& self,
          std::string_view storage_type_text,
          std::size_t target_offset,
          std::string_view repeated_type_text) -> std::optional<RepeatedAggregateExtent> {
    const auto storage_layout = compute_aggregate_type_layout(storage_type_text, type_decls);
    if (storage_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        target_offset >= storage_layout.size_bytes) {
      return std::nullopt;
    }

    switch (storage_layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto nested_element_layout =
            compute_aggregate_type_layout(storage_layout.element_type_text, type_decls);
        if (nested_element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            nested_element_layout.size_bytes == 0) {
          return std::nullopt;
        }
        const auto element_index = target_offset / nested_element_layout.size_bytes;
        if (element_index >= storage_layout.array_count) {
          return std::nullopt;
        }
        const auto nested_offset = target_offset % nested_element_layout.size_bytes;
        if (nested_offset == 0 &&
            c4c::codegen::lir::trim_lir_arg_text(storage_layout.element_type_text) ==
                c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
          return RepeatedAggregateExtent{
              .element_count = storage_layout.array_count - element_index,
              .element_stride_bytes = nested_element_layout.size_bytes,
          };
        }
        return self(self, storage_layout.element_type_text, nested_offset, repeated_type_text);
      }
      case AggregateTypeLayout::Kind::Struct:
        for (std::size_t index = 0; index < storage_layout.fields.size(); ++index) {
          const auto field_begin = storage_layout.fields[index].byte_offset;
          const auto field_end =
              index + 1 < storage_layout.fields.size() ? storage_layout.fields[index + 1].byte_offset
                                                       : storage_layout.size_bytes;
          if (target_offset < field_begin || target_offset >= field_end) {
            continue;
          }
          return self(self,
                      storage_layout.fields[index].type_text,
                      target_offset - field_begin,
                      repeated_type_text);
        }
        return std::nullopt;
      default:
        return std::nullopt;
    }
  };
  if (const auto extent = find_repeated_extent(find_repeated_extent,
                                               aggregate_slots.storage_type_text,
                                               aggregate_slots.base_byte_offset,
                                               type_text);
      extent.has_value() && extent->element_stride_bytes == layout.size_bytes) {
    slot_count *= extent->element_count;
  }

  element_slots.reserve(slot_count);
  for (std::size_t index = 0; index < slot_count; ++index) {
    const auto slot_it = aggregate_slots.leaf_slots.find(
        aggregate_slots.base_byte_offset + index * element_layout.size_bytes);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    element_slots.push_back(slot_it->second);
  }
  return element_slots;
}

bool BirFunctionLowerer::is_local_array_element_slot(std::string_view slot_name,
                                                     const LocalArraySlotMap& local_array_slots) {
  for (const auto& [array_name, array_slots] : local_array_slots) {
    (void)array_name;
    if (std::find(array_slots.element_slots.begin(),
                  array_slots.element_slots.end(),
                  slot_name) != array_slots.element_slots.end()) {
      return true;
    }
  }
  return false;
}

std::optional<std::pair<std::size_t, bir::TypeKind>> BirFunctionLowerer::parse_local_array_type(
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

  const auto element_type =
      lower_scalar_or_function_pointer_type(text.substr(x_pos + 3, text.size() - x_pos - 4));
  if (!element_type.has_value()) {
    return std::nullopt;
  }

  return std::pair<std::size_t, bir::TypeKind>{static_cast<std::size_t>(*count), *element_type};
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

std::optional<LocalAggregateGepTarget> BirFunctionLowerer::resolve_local_aggregate_gep_target(
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

std::optional<DynamicLocalAggregateArrayAccess>
BirFunctionLowerer::build_dynamic_local_aggregate_array_access(
    bir::TypeKind element_type,
    std::size_t byte_offset,
    const LocalAggregateSlots& aggregate_slots,
    const bir::Value& index,
    const TypeDeclMap& type_decls) {
  if (element_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  const auto element_type_text = render_type(element_type);
  const auto extent = find_repeated_aggregate_extent_at_offset(
      aggregate_slots.storage_type_text, byte_offset, element_type_text, type_decls);
  if (!extent.has_value()) {
    return std::nullopt;
  }

  return DynamicLocalAggregateArrayAccess{
      .element_type_text = std::move(element_type_text),
      .byte_offset = byte_offset,
      .element_count = extent->element_count,
      .element_stride_bytes = extent->element_stride_bytes,
      .leaf_slots = aggregate_slots.leaf_slots,
      .index = index,
  };
}

std::optional<DynamicLocalPointerArrayAccess>
BirFunctionLowerer::resolve_dynamic_local_aggregate_gep_projection(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const DynamicLocalAggregateArrayAccess& access) {
  std::vector<std::string> element_slots;
  element_slots.reserve(access.element_count);
  for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
    LocalAggregateSlots element_aggregate{
        .storage_type_text = access.element_type_text,
        .type_text = access.element_type_text,
        .base_byte_offset = access.byte_offset + element_index * access.element_stride_bytes,
        .leaf_slots = access.leaf_slots,
    };
    const auto element_slot = resolve_local_aggregate_gep_slot(
        element_aggregate.type_text, gep, value_aliases, type_decls, element_aggregate);
    if (!element_slot.has_value()) {
      return std::nullopt;
    }
    element_slots.push_back(*element_slot);
  }

  return DynamicLocalPointerArrayAccess{
      .element_slots = std::move(element_slots),
      .index = access.index,
  };
}

std::optional<bir::Value> BirFunctionLowerer::load_dynamic_local_aggregate_array_value(
    std::string_view result_name,
    bir::TypeKind value_type,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_count == 0) {
    return std::nullopt;
  }

  const auto element_layout = compute_aggregate_type_layout(access.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
      element_layout.scalar_type != value_type) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(access.element_count);
  for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
    const auto leaf_offset = access.byte_offset + element_index * access.element_stride_bytes;
    const auto leaf_slot_it = access.leaf_slots.find(leaf_offset);
    if (leaf_slot_it == access.leaf_slots.end()) {
      return std::nullopt;
    }

    const auto slot_it = local_slot_types.find(leaf_slot_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != value_type) {
      return std::nullopt;
    }

    const std::string element_name = std::string(result_name) + ".elt" + std::to_string(element_index);
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(value_type, element_name),
        .slot_name = leaf_slot_it->second,
    });
    element_values.push_back(bir::Value::named(value_type, element_name));
  }

  return synthesize_value_array_selects(result_name, element_values, access.index, lowered_insts);
}

bool BirFunctionLowerer::append_dynamic_local_aggregate_store(
    std::string_view scratch_prefix,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_count == 0) {
    return false;
  }

  const auto element_layout = compute_aggregate_type_layout(access.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
      element_layout.scalar_type != value_type) {
    return false;
  }

  for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
    const auto leaf_offset = access.byte_offset + element_index * access.element_stride_bytes;
    const auto leaf_slot_it = access.leaf_slots.find(leaf_offset);
    if (leaf_slot_it == access.leaf_slots.end()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(leaf_slot_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != value_type) {
      return false;
    }

    const std::string element_name =
        std::string(scratch_prefix) + ".elt" + std::to_string(element_index);
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(value_type, element_name),
        .slot_name = leaf_slot_it->second,
    });

    bir::Value stored_value = value;
    if (access.element_count > 1) {
      const auto compare_rhs = make_index_immediate(access.index.type, element_index);
      if (!compare_rhs.has_value()) {
        return false;
      }
      const std::string select_name =
          std::string(scratch_prefix) + ".store" + std::to_string(element_index);
      lowered_insts->push_back(bir::SelectInst{
          .predicate = bir::BinaryOpcode::Eq,
          .result = bir::Value::named(value_type, select_name),
          .compare_type = access.index.type,
          .lhs = access.index,
          .rhs = *compare_rhs,
          .true_value = value,
          .false_value = bir::Value::named(value_type, element_name),
      });
      stored_value = bir::Value::named(value_type, select_name);
    }

    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = leaf_slot_it->second,
        .value = stored_value,
    });
  }
  return true;
}

}  // namespace c4c::backend
