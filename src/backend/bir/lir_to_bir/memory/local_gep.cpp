#include "../lowering.hpp"
#include "memory_helpers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using DynamicLocalPointerArrayAccess = BirFunctionLowerer::DynamicLocalPointerArrayAccess;
using LocalAggregateGepTarget = BirFunctionLowerer::LocalAggregateGepTarget;
using BackendStructuredLayoutTable = lir_to_bir_detail::BackendStructuredLayoutTable;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::lookup_backend_aggregate_type_layout;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

namespace {

struct LocalAggregateRawByteSliceLeaf {
  std::size_t byte_offset = 0;
  std::string type_text;
};

BirFunctionLowerer::AggregateTypeLayout lookup_local_gep_layout(
    std::string_view type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (structured_layouts != nullptr) {
    return lookup_backend_aggregate_type_layout(type_text, type_decls, *structured_layouts);
  }
  return compute_aggregate_type_layout(type_text, type_decls);
}

std::optional<AggregateByteOffsetProjection> resolve_local_gep_child_index_projection(
    std::string_view type_text,
    std::size_t child_index,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  return structured_layouts != nullptr
             ? resolve_aggregate_child_index_projection(
                   type_text, child_index, type_decls, *structured_layouts)
             : resolve_aggregate_child_index_projection(type_text, child_index, type_decls);
}

bool can_reinterpret_local_gep_byte_storage_as_type(
    std::string_view storage_type_text,
    std::size_t target_byte_offset,
    std::string_view target_type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  return structured_layouts != nullptr
             ? can_reinterpret_byte_storage_as_type(storage_type_text,
                                                    target_byte_offset,
                                                    target_type_text,
                                                    type_decls,
                                                    *structured_layouts)
             : can_reinterpret_byte_storage_as_type(storage_type_text,
                                                    target_byte_offset,
                                                    target_type_text,
                                                    type_decls);
}

std::optional<LocalAggregateRawByteSliceLeaf> resolve_local_aggregate_raw_byte_slice_leaf(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const BirFunctionLowerer::ValueMap& value_aliases,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const BirFunctionLowerer::LocalAggregateSlots& aggregate_slots) {
  if (c4c::codegen::lir::trim_lir_arg_text(gep.element_type.str()) != "i8") {
    return std::nullopt;
  }

  std::size_t raw_index_pos = 0;
  if (gep.indices.size() == 2) {
    const auto base_index = parse_typed_operand(gep.indices.front());
    if (!base_index.has_value()) {
      return std::nullopt;
    }
    const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
    if (!base_imm.has_value() || *base_imm != 0) {
      return std::nullopt;
    }
    raw_index_pos = 1;
  } else if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto parsed_index = parse_typed_operand(gep.indices[raw_index_pos]);
  if (!parsed_index.has_value()) {
    return std::nullopt;
  }
  const auto byte_index = resolve_index_operand(parsed_index->operand, value_aliases);
  if (!byte_index.has_value() || *byte_index < 0) {
    return std::nullopt;
  }

  const auto base_layout = lookup_local_gep_layout(base_type_text, type_decls, structured_layouts);
  if ((base_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Struct &&
       base_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Array) ||
      static_cast<std::size_t>(*byte_index) >= base_layout.size_bytes) {
    return std::nullopt;
  }

  const auto scalar_facts = resolve_scalar_layout_facts_at_byte_offset(
      base_type_text, static_cast<std::size_t>(*byte_index), type_decls);
  if (!scalar_facts.has_value() || !scalar_facts->leaf.has_value()) {
    return std::nullopt;
  }

  const auto absolute_byte_offset =
      aggregate_slots.base_byte_offset + static_cast<std::size_t>(*byte_index);
  return aggregate_slots.leaf_slots.find(absolute_byte_offset) != aggregate_slots.leaf_slots.end()
             ? std::optional<LocalAggregateRawByteSliceLeaf>(LocalAggregateRawByteSliceLeaf{
                   .byte_offset = absolute_byte_offset,
                   .type_text = scalar_facts->leaf->type_text,
               })
             : std::nullopt;
}

}  // namespace

std::optional<std::string> BirFunctionLowerer::resolve_local_aggregate_gep_slot(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  return resolve_local_aggregate_gep_slot(
      base_type_text, gep, value_aliases, type_decls, nullptr, aggregate_slots);
}

std::optional<std::string> BirFunctionLowerer::resolve_local_aggregate_gep_slot(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalAggregateSlots& aggregate_slots) {
  if (const auto raw_byte_slice_leaf = resolve_local_aggregate_raw_byte_slice_leaf(
          base_type_text, gep, value_aliases, type_decls, structured_layouts, aggregate_slots);
      raw_byte_slice_leaf.has_value()) {
    const auto slot_it = aggregate_slots.leaf_slots.find(raw_byte_slice_leaf->byte_offset);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    return slot_it->second;
  }

  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(base_type_text));
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

    const auto layout = lookup_local_gep_layout(current_type, type_decls, structured_layouts);
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
          can_reinterpret_local_gep_byte_storage_as_type(
              current_type, 0, gep_element_type, type_decls, structured_layouts)) {
        current_type = gep_element_type;
        continue;
      }
    }

    const auto projection = resolve_local_gep_child_index_projection(
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

  const auto leaf_layout = lookup_local_gep_layout(current_type, type_decls, structured_layouts);
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
  return resolve_local_aggregate_pointer_array_slots(
      base_type_text, gep, value_aliases, type_decls, nullptr, aggregate_slots);
}

std::optional<std::vector<std::string>>
BirFunctionLowerer::resolve_local_aggregate_pointer_array_slots(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalAggregateSlots& aggregate_slots) {
  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(base_type_text));
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

    const auto layout = lookup_local_gep_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (*index_value == 0 && gep_element_type != current_type &&
          can_reinterpret_local_gep_byte_storage_as_type(
              current_type, 0, gep_element_type, type_decls, structured_layouts)) {
        current_type = gep_element_type;
        continue;
      }
      if (*index_value != 0) {
        const auto extent =
            structured_layouts != nullptr
                ? find_repeated_aggregate_extent_at_offset(aggregate_slots.storage_type_text,
                                                           aggregate_slots.base_byte_offset,
                                                           base_type_text,
                                                           type_decls,
                                                           *structured_layouts)
                : find_repeated_aggregate_extent_at_offset(aggregate_slots.storage_type_text,
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

    const auto projection = resolve_local_gep_child_index_projection(
        current_type, static_cast<std::size_t>(*index_value), type_decls, structured_layouts);
    if (!projection.has_value()) {
      return std::nullopt;
    }
    switch (projection->kind) {
      case AggregateByteOffsetProjection::Kind::ArrayElement:
        if (projection->child_layout.kind == AggregateTypeLayout::Kind::Scalar &&
            projection->child_layout.size_bytes != 0 && *index_value == 0 &&
            &raw_index == &gep.indices.back()) {
          std::vector<std::string> element_slots;
          element_slots.reserve(projection->layout.array_count);
          for (std::size_t index = 0; index < projection->layout.array_count; ++index) {
            const auto slot_it = aggregate_slots.leaf_slots.find(
                byte_offset + index * projection->child_layout.size_bytes);
            if (slot_it == aggregate_slots.leaf_slots.end()) {
              return std::nullopt;
            }
            element_slots.push_back(slot_it->second);
          }
          return element_slots;
        }
        [[fallthrough]];
      case AggregateByteOffsetProjection::Kind::StructField:
        byte_offset += projection->child_start_byte_offset;
        current_type = projection->child_type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto layout = lookup_local_gep_layout(current_type, type_decls, structured_layouts);
  if (layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout =
      lookup_local_gep_layout(layout.element_type_text, type_decls, structured_layouts);
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
  return resolve_local_aggregate_dynamic_pointer_array_access(
      base_type_text, gep, value_aliases, type_decls, nullptr, aggregate_slots);
}

std::optional<DynamicLocalPointerArrayAccess>
BirFunctionLowerer::resolve_local_aggregate_dynamic_pointer_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalAggregateSlots& aggregate_slots) {
  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(base_type_text));
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = lookup_local_gep_layout(current_type, type_decls, structured_layouts);
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

    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (index_value.has_value()) {
      if (*index_value < 0) {
        return std::nullopt;
      }
      const auto projection = resolve_local_gep_child_index_projection(
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
            lookup_local_gep_layout(layout.element_type_text, type_decls, structured_layouts);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value()) {
          return std::nullopt;
        }

        std::size_t element_leaf_offset = 0;
        std::string element_type(c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text));
        for (std::size_t tail_pos = index_pos + 1; tail_pos < gep.indices.size(); ++tail_pos) {
          const auto tail_index = parse_typed_operand(gep.indices[tail_pos]);
          if (!tail_index.has_value()) {
            return std::nullopt;
          }
          const auto tail_value = resolve_index_operand(tail_index->operand, value_aliases);
          if (!tail_value.has_value() || *tail_value < 0) {
            return std::nullopt;
          }

          const auto tail_layout =
              lookup_local_gep_layout(element_type, type_decls, structured_layouts);
          if (tail_layout.kind == AggregateTypeLayout::Kind::Invalid ||
              tail_layout.size_bytes == 0 || tail_layout.align_bytes == 0) {
            return std::nullopt;
          }

          const auto projection = resolve_local_gep_child_index_projection(
              element_type, static_cast<std::size_t>(*tail_value), type_decls, structured_layouts);
          if (!projection.has_value()) {
            return std::nullopt;
          }
          switch (projection->kind) {
            case AggregateByteOffsetProjection::Kind::ArrayElement:
            case AggregateByteOffsetProjection::Kind::StructField:
              element_leaf_offset += projection->child_start_byte_offset;
              element_type = projection->child_type_text;
              break;
            default:
              return std::nullopt;
          }
        }

        const auto final_layout =
            lookup_local_gep_layout(element_type, type_decls, structured_layouts);
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
  return resolve_local_aggregate_gep_target(
      base_type_text, gep, value_aliases, type_decls, nullptr, aggregate_slots);
}

std::optional<LocalAggregateGepTarget> BirFunctionLowerer::resolve_local_aggregate_gep_target(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalAggregateSlots& aggregate_slots) {
  if (const auto raw_byte_slice_leaf = resolve_local_aggregate_raw_byte_slice_leaf(
          base_type_text, gep, value_aliases, type_decls, structured_layouts, aggregate_slots);
      raw_byte_slice_leaf.has_value()) {
    return LocalAggregateGepTarget{
        .type_text = raw_byte_slice_leaf->type_text,
        .byte_offset = static_cast<std::int64_t>(raw_byte_slice_leaf->byte_offset),
    };
  }

  std::string current_type(c4c::codegen::lir::trim_lir_arg_text(base_type_text));
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

    const auto layout = lookup_local_gep_layout(current_type, type_decls, structured_layouts);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (gep_element_type == current_type && *index_value != 0) {
        const auto extent =
            structured_layouts != nullptr
                ? find_repeated_aggregate_extent_at_offset(aggregate_slots.storage_type_text,
                                                           aggregate_slots.base_byte_offset,
                                                           base_type_text,
                                                           type_decls,
                                                           *structured_layouts)
                : find_repeated_aggregate_extent_at_offset(aggregate_slots.storage_type_text,
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
          can_reinterpret_local_gep_byte_storage_as_type(
              current_type, 0, gep_element_type, type_decls, structured_layouts)) {
        current_type = gep_element_type;
        continue;
      }
    }

    const auto projection = resolve_local_gep_child_index_projection(
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
  return resolve_local_dynamic_aggregate_array_access(
      base_type_text, gep, value_aliases, type_decls, nullptr, aggregate_slots);
}

std::optional<DynamicLocalAggregateArrayAccess>
BirFunctionLowerer::resolve_local_dynamic_aggregate_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
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

  const auto extent =
      structured_layouts != nullptr
          ? find_repeated_aggregate_extent_at_offset(aggregate_slots.storage_type_text,
                                                     aggregate_slots.base_byte_offset,
                                                     base_type_text,
                                                     type_decls,
                                                     *structured_layouts)
          : find_repeated_aggregate_extent_at_offset(aggregate_slots.storage_type_text,
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
  return build_dynamic_local_aggregate_array_access(
      element_type, byte_offset, aggregate_slots, index, type_decls, nullptr);
}

std::optional<DynamicLocalAggregateArrayAccess>
BirFunctionLowerer::build_dynamic_local_aggregate_array_access(
    bir::TypeKind element_type,
    std::size_t byte_offset,
    const LocalAggregateSlots& aggregate_slots,
    const bir::Value& index,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (element_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  const auto element_type_text = render_type(element_type);
  const auto extent =
      structured_layouts != nullptr
          ? find_repeated_aggregate_extent_at_offset(
                aggregate_slots.storage_type_text,
                byte_offset,
                element_type_text,
                type_decls,
                *structured_layouts)
          : find_repeated_aggregate_extent_at_offset(
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
  return resolve_dynamic_local_aggregate_gep_projection(
      gep, value_aliases, type_decls, nullptr, access);
}

std::optional<DynamicLocalPointerArrayAccess>
BirFunctionLowerer::resolve_dynamic_local_aggregate_gep_projection(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
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
        element_aggregate.type_text,
        gep,
        value_aliases,
        type_decls,
        structured_layouts,
        element_aggregate);
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

std::optional<bool> BirFunctionLowerer::try_lower_dynamic_local_aggregate_gep_projection(
    const c4c::codegen::lir::LirGepOp& gep,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays) {
  return try_lower_dynamic_local_aggregate_gep_projection(
      gep,
      dynamic_local_aggregate_arrays,
      value_aliases,
      type_decls,
      nullptr,
      dynamic_local_pointer_arrays);
}

std::optional<bool> BirFunctionLowerer::try_lower_dynamic_local_aggregate_gep_projection(
    const c4c::codegen::lir::LirGepOp& gep,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays) {
  const auto dynamic_local_aggregate_it =
      dynamic_local_aggregate_arrays.find(std::string(gep.ptr.str()));
  if (dynamic_local_aggregate_it == dynamic_local_aggregate_arrays.end()) {
    return std::nullopt;
  }

  const auto projection = resolve_dynamic_local_aggregate_gep_projection(
      gep, value_aliases, type_decls, structured_layouts, dynamic_local_aggregate_it->second);
  if (!projection.has_value()) {
    return false;
  }

  (*dynamic_local_pointer_arrays)[std::string(gep.result.str())] = std::move(*projection);
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_local_slot_pointer_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    LocalSlotPointerValues* local_slot_pointer_values) {
  return try_lower_local_slot_pointer_gep(
      gep, value_aliases, type_decls, nullptr, local_slot_types, local_slot_pointer_values);
}

std::optional<bool> BirFunctionLowerer::try_lower_local_slot_pointer_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalSlotTypes& local_slot_types,
    LocalSlotPointerValues* local_slot_pointer_values) {
  const auto local_slot_ptr_it = local_slot_pointer_values->find(std::string(gep.ptr.str()));
  if (local_slot_ptr_it == local_slot_pointer_values->end()) {
    return std::nullopt;
  }

  auto resolved_target = resolve_relative_gep_target(local_slot_ptr_it->second.type_text,
                                                     local_slot_ptr_it->second.byte_offset,
                                                     gep,
                                                     value_aliases,
                                                     type_decls,
                                                     structured_layouts);
  if (!local_slot_ptr_it->second.array_element_slots.empty() && gep.indices.size() == 1) {
    const auto parsed_index = parse_typed_operand(gep.indices.front());
    if (!parsed_index.has_value()) {
      return false;
    }
    const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_imm.has_value()) {
      return false;
    }
    const auto& base_slot = local_slot_ptr_it->second.array_element_slots.front();
    const auto slot_type_it = local_slot_types.find(base_slot);
    if (slot_type_it == local_slot_types.end()) {
      return false;
    }
    const auto slot_size = type_size_bytes(slot_type_it->second);
    if (slot_size == 0) {
      return false;
    }
    const auto final_byte_offset =
        local_slot_ptr_it->second.byte_offset + *index_imm * static_cast<std::int64_t>(slot_size);
    if (!resolved_target.has_value()) {
      resolved_target = LocalAggregateGepTarget{
          .type_text = render_type(slot_type_it->second),
          .byte_offset = final_byte_offset,
      };
    }
  }
  if (!resolved_target.has_value()) {
    return false;
  }

  const auto leaf_layout =
      lookup_local_gep_layout(resolved_target->type_text, type_decls, structured_layouts);
  LocalSlotAddress resolved_address{
      .slot_name = local_slot_ptr_it->second.slot_name,
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = resolved_target->byte_offset,
      .storage_type_text = local_slot_ptr_it->second.storage_type_text,
      .type_text = std::move(resolved_target->type_text),
  };
  if (!local_slot_ptr_it->second.array_element_slots.empty()) {
    const auto& base_slot = local_slot_ptr_it->second.array_element_slots.front();
    const auto slot_type_it = local_slot_types.find(base_slot);
    if (slot_type_it != local_slot_types.end()) {
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size != 0 && resolved_address.value_type == slot_type_it->second &&
          resolved_address.byte_offset >= 0 &&
          resolved_address.byte_offset % slot_size == 0 &&
          static_cast<std::size_t>(resolved_address.byte_offset / slot_size) <=
              local_slot_ptr_it->second.array_element_slots.size()) {
        resolved_address.slot_name = base_slot;
        resolved_address.storage_type_text = render_type(slot_type_it->second);
        resolved_address.type_text = render_type(slot_type_it->second);
        resolved_address.array_element_slots = local_slot_ptr_it->second.array_element_slots;
        resolved_address.array_base_index =
            static_cast<std::size_t>(resolved_address.byte_offset / slot_size);
      } else {
        resolved_address.slot_name = base_slot;
        resolved_address.storage_type_text = render_type(slot_type_it->second);
        resolved_address.type_text = render_type(slot_type_it->second);
        resolved_address.array_element_slots = local_slot_ptr_it->second.array_element_slots;
      }
    }
  }

  (*local_slot_pointer_values)[std::string(gep.result.str())] = std::move(resolved_address);
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_local_array_slot_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const LocalArraySlotMap& local_array_slots,
    LocalPointerSlots* local_pointer_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays) {
  const auto array_it = local_array_slots.find(std::string(gep.ptr.str()));
  if (array_it == local_array_slots.end()) {
    return std::nullopt;
  }

  if (gep.indices.size() != 2) {
    return false;
  }

  const auto base_index = parse_typed_operand(gep.indices[0]);
  const auto elem_index = parse_typed_operand(gep.indices[1]);
  if (!base_index.has_value() || !elem_index.has_value()) {
    return false;
  }

  const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
  const auto elem_imm = resolve_index_operand(elem_index->operand, value_aliases);
  if (!base_imm.has_value() || *base_imm != 0) {
    return false;
  }

  const std::string result_name(gep.result.str());
  if (elem_imm.has_value()) {
    if (*elem_imm < 0 ||
        static_cast<std::size_t>(*elem_imm) >= array_it->second.element_slots.size()) {
      return false;
    }
    (*local_pointer_slots)[result_name] =
        array_it->second.element_slots[static_cast<std::size_t>(*elem_imm)];
    (*local_pointer_array_bases)[result_name] = LocalPointerArrayBase{
        .element_slots = array_it->second.element_slots,
        .base_index = static_cast<std::size_t>(*elem_imm),
    };
    return true;
  }

  const auto elem_value = lower_typed_index_value(*elem_index, value_aliases);
  if (!elem_value.has_value() || array_it->second.element_type != bir::TypeKind::Ptr) {
    return false;
  }

  (*dynamic_local_pointer_arrays)[result_name] = DynamicLocalPointerArrayAccess{
      .element_slots = array_it->second.element_slots,
      .index = *elem_value,
  };
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_local_pointer_array_base_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const LocalSlotTypes& local_slot_types,
    LocalPointerSlots* local_pointer_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
    DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays,
    LocalSlotPointerValues* local_slot_pointer_values) {
  return try_lower_local_pointer_array_base_gep(gep,
                                               value_aliases,
                                               TypeDeclMap{},
                                               nullptr,
                                               local_slot_types,
                                               local_pointer_slots,
                                               local_pointer_array_bases,
                                               dynamic_local_pointer_arrays,
                                               dynamic_local_aggregate_arrays,
                                               local_slot_pointer_values);
}

std::optional<bool> BirFunctionLowerer::try_lower_local_pointer_array_base_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalSlotTypes& local_slot_types,
    LocalPointerSlots* local_pointer_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
    DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays,
    LocalSlotPointerValues* local_slot_pointer_values) {
  (void)type_decls;
  (void)structured_layouts;
  const auto array_base_it = local_pointer_array_bases->find(std::string(gep.ptr.str()));
  if (array_base_it == local_pointer_array_bases->end()) {
    return std::nullopt;
  }

  if (gep.indices.empty() || gep.indices.size() > 2) {
    return false;
  }

  std::size_t index_pos = 0;
  if (gep.indices.size() == 2) {
    const auto base_index = parse_typed_operand(gep.indices[0]);
    if (!base_index.has_value()) {
      return false;
    }
    const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
    if (!base_imm.has_value() || *base_imm != 0) {
      return false;
    }
    index_pos = 1;
  }

  const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
  if (!parsed_index.has_value()) {
    return false;
  }

  const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
  if (index_imm.has_value()) {
    const auto final_index = static_cast<std::int64_t>(array_base_it->second.base_index) + *index_imm;
    if (final_index < 0) {
      if (array_base_it->second.element_slots.empty()) {
        return false;
      }
      const auto& base_slot = array_base_it->second.element_slots.front();
      const auto slot_type_it = local_slot_types.find(base_slot);
      if (slot_type_it == local_slot_types.end()) {
        return false;
      }
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return false;
      }
      (*local_slot_pointer_values)[std::string(gep.result.str())] = LocalSlotAddress{
          .slot_name = base_slot,
          .value_type = slot_type_it->second,
          .byte_offset = final_index * static_cast<std::int64_t>(slot_size),
          .storage_type_text = render_type(slot_type_it->second),
          .type_text = render_type(slot_type_it->second),
          .array_element_slots = array_base_it->second.element_slots,
      };
      return true;
    }

    if (static_cast<std::size_t>(final_index) > array_base_it->second.element_slots.size()) {
      return false;
    }

    if (static_cast<std::size_t>(final_index) == array_base_it->second.element_slots.size()) {
      if (array_base_it->second.element_slots.empty()) {
        return false;
      }
      const auto& base_slot = array_base_it->second.element_slots.front();
      const auto slot_type_it = local_slot_types.find(base_slot);
      if (slot_type_it == local_slot_types.end()) {
        return false;
      }
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return false;
      }
      (*local_slot_pointer_values)[std::string(gep.result.str())] = LocalSlotAddress{
          .slot_name = base_slot,
          .value_type = slot_type_it->second,
          .byte_offset = final_index * static_cast<std::int64_t>(slot_size),
          .storage_type_text = render_type(slot_type_it->second),
          .type_text = render_type(slot_type_it->second),
          .array_element_slots = array_base_it->second.element_slots,
          .array_base_index = static_cast<std::size_t>(final_index),
      };
      (*local_pointer_array_bases)[std::string(gep.result.str())] = LocalPointerArrayBase{
          .element_slots = array_base_it->second.element_slots,
          .base_index = static_cast<std::size_t>(final_index),
      };
      return true;
    }

    (*local_pointer_slots)[std::string(gep.result.str())] =
        array_base_it->second.element_slots[static_cast<std::size_t>(final_index)];
    (*local_pointer_array_bases)[std::string(gep.result.str())] = LocalPointerArrayBase{
        .element_slots = array_base_it->second.element_slots,
        .base_index = static_cast<std::size_t>(final_index),
    };
    return true;
  }

  const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
  if (!index_value.has_value() || array_base_it->second.base_index != 0) {
    return false;
  }
  if (array_base_it->second.element_slots.empty()) {
    return false;
  }

  const auto slot_type_it = local_slot_types.find(array_base_it->second.element_slots.front());
  if (slot_type_it == local_slot_types.end()) {
    return false;
  }

  if (slot_type_it->second == bir::TypeKind::Ptr) {
    (*dynamic_local_pointer_arrays)[std::string(gep.result.str())] = DynamicLocalPointerArrayAccess{
        .element_slots = array_base_it->second.element_slots,
        .index = *index_value,
    };
    return true;
  }

  const auto element_size = type_size_bytes(slot_type_it->second);
  if (element_size == 0) {
    return false;
  }

  std::optional<std::string> element_type_text;
  switch (slot_type_it->second) {
    case bir::TypeKind::I1:
      element_type_text = "i1";
      break;
    case bir::TypeKind::I8:
      element_type_text = "i8";
      break;
    case bir::TypeKind::I32:
      element_type_text = "i32";
      break;
    case bir::TypeKind::I64:
      element_type_text = "i64";
      break;
    default:
      return false;
  }

  DynamicLocalAggregateArrayAccess access{
      .element_type_text = *element_type_text,
      .byte_offset = 0,
      .element_count = array_base_it->second.element_slots.size(),
      .element_stride_bytes = element_size,
      .index = *index_value,
  };
  for (std::size_t element_index = 0; element_index < array_base_it->second.element_slots.size();
       ++element_index) {
    const auto& slot_name = array_base_it->second.element_slots[element_index];
    const auto element_slot_type_it = local_slot_types.find(slot_name);
    if (element_slot_type_it == local_slot_types.end() ||
        element_slot_type_it->second != slot_type_it->second) {
      return false;
    }
    access.leaf_slots.emplace(element_index * element_size, slot_name);
  }

  (*dynamic_local_aggregate_arrays)[std::string(gep.result.str())] = std::move(access);
  return true;
}

std::optional<bool> BirFunctionLowerer::try_lower_local_pointer_slot_base_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    const LocalArraySlotMap& local_array_slots,
    const LocalAggregateSlotMap& local_aggregate_slots,
    LocalPointerSlots* local_pointer_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
    DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays) {
  return try_lower_local_pointer_slot_base_gep(gep,
                                              value_aliases,
                                              type_decls,
                                              nullptr,
                                              local_slot_types,
                                              local_array_slots,
                                              local_aggregate_slots,
                                              local_pointer_slots,
                                              local_pointer_array_bases,
                                              dynamic_local_pointer_arrays,
                                              dynamic_local_aggregate_arrays);
}

std::optional<bool> BirFunctionLowerer::try_lower_local_pointer_slot_base_gep(
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalSlotTypes& local_slot_types,
    const LocalArraySlotMap& local_array_slots,
    const LocalAggregateSlotMap& local_aggregate_slots,
    LocalPointerSlots* local_pointer_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
    DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays) {
  const auto ptr_it = local_pointer_slots->find(std::string(gep.ptr.str()));
  if (ptr_it == local_pointer_slots->end()) {
    return std::nullopt;
  }
  if (gep.indices.size() != 1) {
    return false;
  }

  const auto slot_it = local_slot_types.find(ptr_it->second);
  if (slot_it == local_slot_types.end()) {
    return false;
  }

  const auto parsed_index = parse_typed_operand(gep.indices.front());
  if (!parsed_index.has_value()) {
    return false;
  }
  const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);

  std::string resolved_slot;
  std::optional<LocalPointerArrayBase> resolved_array_base;
  const auto dot = ptr_it->second.rfind('.');
  if (dot == std::string::npos) {
    if (!index_imm.has_value() || *index_imm != 0) {
      return false;
    }
    resolved_slot = ptr_it->second;
  } else {
    const auto base_name = ptr_it->second.substr(0, dot);
    const auto base_array_it = local_array_slots.find(base_name);
    const auto base_offset = parse_i64(std::string_view(ptr_it->second).substr(dot + 1));
    if (!base_offset.has_value()) {
      return false;
    }
    if (base_array_it != local_array_slots.end()) {
      if (index_imm.has_value()) {
        const auto final_index = *base_offset + *index_imm;
        if (final_index < 0 ||
            static_cast<std::size_t>(final_index) >= base_array_it->second.element_slots.size()) {
          return false;
        }
        resolved_slot = base_array_it->second.element_slots[static_cast<std::size_t>(final_index)];
        resolved_array_base = LocalPointerArrayBase{
            .element_slots = base_array_it->second.element_slots,
            .base_index = static_cast<std::size_t>(final_index),
        };
      } else {
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value() || *base_offset != 0 ||
            base_array_it->second.element_type != bir::TypeKind::Ptr) {
          return false;
        }
        (*dynamic_local_pointer_arrays)[std::string(gep.result.str())] = DynamicLocalPointerArrayAccess{
            .element_slots = base_array_it->second.element_slots,
            .index = *index_value,
        };
        return true;
      }
    } else if (const auto base_aggregate_it = local_aggregate_slots.find(base_name);
               base_aggregate_it != local_aggregate_slots.end()) {
      const auto slot_size = type_size_bytes(slot_it->second);
      if (slot_size == 0) {
        return false;
      }
      if (!index_imm.has_value()) {
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value() || *base_offset < 0) {
          return false;
        }

        const auto dynamic_access = build_dynamic_local_aggregate_array_access(
            slot_it->second,
            static_cast<std::size_t>(*base_offset),
            base_aggregate_it->second,
            *index_value,
            type_decls,
            structured_layouts);
        if (!dynamic_access.has_value()) {
          return false;
        }

        (*dynamic_local_aggregate_arrays)[std::string(gep.result.str())] = std::move(*dynamic_access);
        return true;
      }
      const auto final_byte_offset =
          *base_offset + *index_imm * static_cast<std::int64_t>(slot_size);
      if (final_byte_offset < 0) {
        return false;
      }
      const auto leaf_it =
          base_aggregate_it->second.leaf_slots.find(static_cast<std::size_t>(final_byte_offset));
      if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
        return false;
      }
      resolved_slot = leaf_it->second;
    } else {
      return false;
    }
  }

  if (resolved_array_base.has_value()) {
    (*local_pointer_array_bases)[std::string(gep.result.str())] = std::move(*resolved_array_base);
  }
  (*local_pointer_slots)[std::string(gep.result.str())] = std::move(resolved_slot);
  return true;
}

}  // namespace c4c::backend
