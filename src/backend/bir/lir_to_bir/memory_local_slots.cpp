#include "lowering.hpp"

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
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

namespace {

struct LocalArrayView {
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::vector<std::string> element_slots;
  std::size_t base_index = 0;
};

struct LocalMemcpyLeaf {
  std::size_t byte_offset = 0;
  bir::TypeKind type = bir::TypeKind::Void;
  std::string slot_name;
};

struct LocalMemcpyLeafView {
  std::size_t size_bytes = 0;
  std::vector<LocalMemcpyLeaf> leaves;
};

struct LocalMemcpyScalarSlot {
  std::string slot_name;
  bir::TypeKind type = bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct LocalMemsetScalarSlot {
  std::string slot_name;
  std::size_t size_bytes = 0;
};

struct LocalAggregateRawByteSliceLeaf {
  std::size_t byte_offset = 0;
  std::string type_text;
};

std::optional<std::string> resolve_scalar_leaf_type_at_byte_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Scalar:
      return target_offset == 0 ? std::optional<std::string>(c4c::backend::bir::render_type(layout.scalar_type))
                                : std::nullopt;
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Array: {
      const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      return resolve_scalar_leaf_type_at_byte_offset(layout.element_type_text,
                                                     nested_offset,
                                                     type_decls);
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
        return resolve_scalar_leaf_type_at_byte_offset(layout.fields[index].type_text,
                                                       target_offset - field_begin,
                                                       type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<LocalAggregateRawByteSliceLeaf> resolve_local_aggregate_raw_byte_slice_leaf(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const BirFunctionLowerer::ValueMap& value_aliases,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
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

  const auto base_layout = compute_aggregate_type_layout(base_type_text, type_decls);
  if ((base_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Struct &&
       base_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Array) ||
      static_cast<std::size_t>(*byte_index) >= base_layout.size_bytes) {
    return std::nullopt;
  }

  const auto leaf_type =
      resolve_scalar_leaf_type_at_byte_offset(base_type_text,
                                              static_cast<std::size_t>(*byte_index),
                                              type_decls);
  if (!leaf_type.has_value()) {
    return std::nullopt;
  }

  const auto absolute_byte_offset =
      aggregate_slots.base_byte_offset + static_cast<std::size_t>(*byte_index);
  return aggregate_slots.leaf_slots.find(absolute_byte_offset) != aggregate_slots.leaf_slots.end()
             ? std::optional<LocalAggregateRawByteSliceLeaf>(LocalAggregateRawByteSliceLeaf{
                   .byte_offset = absolute_byte_offset,
                   .type_text = *leaf_type,
               })
             : std::nullopt;
}

}  // namespace

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

bool BirFunctionLowerer::try_lower_immediate_local_memset(
    std::string_view dst_operand,
    std::uint8_t fill_byte,
    std::size_t fill_size_bytes,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    const LocalPointerSlots& local_pointer_slots,
    const LocalArraySlotMap& local_array_slots,
    const LocalPointerArrayBaseMap& local_pointer_array_bases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    std::vector<bir::Inst>* lowered_insts) {
  const auto resolve_local_array_view =
      [&](std::string_view operand) -> std::optional<LocalArrayView> {
    if (const auto array_it = local_array_slots.find(std::string(operand));
        array_it != local_array_slots.end()) {
      return LocalArrayView{
          .element_type = array_it->second.element_type,
          .element_slots = array_it->second.element_slots,
          .base_index = 0,
      };
    }
    if (const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
        array_base_it != local_pointer_array_bases.end() &&
        array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
      const auto slot_type_it =
          local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
      if (slot_type_it != local_slot_types.end()) {
        return LocalArrayView{
            .element_type = slot_type_it->second,
            .element_slots = array_base_it->second.element_slots,
            .base_index = array_base_it->second.base_index,
        };
      }
    }

    const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
    if (ptr_slot_it == local_pointer_slots.end()) {
      return std::nullopt;
    }
    const auto dot = ptr_slot_it->second.rfind('.');
    if (dot == std::string::npos) {
      return std::nullopt;
    }

    const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
    if (slot_type_it == local_slot_types.end()) {
      return std::nullopt;
    }

    const std::string base_name = ptr_slot_it->second.substr(0, dot);
    const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
    if (!base_offset.has_value() || *base_offset < 0) {
      return std::nullopt;
    }

    if (const auto base_array_it = local_array_slots.find(base_name);
        base_array_it != local_array_slots.end()) {
      const auto base_index = static_cast<std::size_t>(*base_offset);
      if (base_index >= base_array_it->second.element_slots.size() ||
          base_array_it->second.element_type != slot_type_it->second) {
        return std::nullopt;
      }
      return LocalArrayView{
          .element_type = slot_type_it->second,
          .element_slots = base_array_it->second.element_slots,
          .base_index = base_index,
      };
    }

    const auto base_aggregate_it = local_aggregate_slots.find(base_name);
    if (base_aggregate_it == local_aggregate_slots.end()) {
      return std::nullopt;
    }

    const auto extent = find_repeated_aggregate_extent_at_offset(
        base_aggregate_it->second.storage_type_text,
        static_cast<std::size_t>(*base_offset),
        render_type(slot_type_it->second),
        type_decls);
    if (!extent.has_value()) {
      return std::nullopt;
    }

    std::vector<std::string> element_slots;
    element_slots.reserve(extent->element_count);
    for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
      const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
          static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
      if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
        return std::nullopt;
      }
      element_slots.push_back(leaf_it->second);
    }

    return LocalArrayView{
        .element_type = slot_type_it->second,
        .element_slots = std::move(element_slots),
        .base_index = 0,
    };
  };
  const auto resolve_local_memset_scalar_slot =
      [&](std::string_view operand) -> std::optional<LocalMemsetScalarSlot> {
    const auto ptr_it = local_pointer_slots.find(std::string(operand));
    if (ptr_it == local_pointer_slots.end()) {
      return std::nullopt;
    }

    const auto slot_type_it = local_slot_types.find(ptr_it->second);
    if (slot_type_it == local_slot_types.end()) {
      return std::nullopt;
    }

    const auto slot_size = type_size_bytes(slot_type_it->second);
    if (slot_size == 0) {
      return std::nullopt;
    }

    return LocalMemsetScalarSlot{
        .slot_name = ptr_it->second,
        .size_bytes = slot_size,
    };
  };
  const auto collect_sorted_leaf_slots_for_memops =
      [&](const LocalAggregateSlots& aggregate_slots) -> std::vector<std::pair<std::size_t, std::string>> {
    const auto layout = lower_byval_aggregate_layout(aggregate_slots.type_text, type_decls);
    if (!layout.has_value()) {
      return {};
    }

    const auto begin_offset = aggregate_slots.base_byte_offset;
    const auto end_offset = begin_offset + layout->size_bytes;
    std::vector<std::pair<std::size_t, std::string>> leaves;
    leaves.reserve(aggregate_slots.leaf_slots.size());
    for (const auto& [byte_offset, slot_name] : aggregate_slots.leaf_slots) {
      if (byte_offset < begin_offset || byte_offset >= end_offset) {
        continue;
      }
      leaves.push_back({byte_offset - begin_offset, slot_name});
    }
    std::sort(leaves.begin(),
              leaves.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    return leaves;
  };
  const auto fill_leaf_slots = [&](const auto& leaf_slots, std::size_t total_size_bytes) -> bool {
    if (fill_size_bytes > total_size_bytes) {
      return false;
    }

    std::size_t covered_bytes = 0;
    for (const auto& [byte_offset, slot_name] : leaf_slots) {
      if (covered_bytes == fill_size_bytes) {
        break;
      }
      if (byte_offset != covered_bytes) {
        return false;
      }

      const auto slot_type_it = local_slot_types.find(slot_name);
      if (slot_type_it == local_slot_types.end()) {
        return false;
      }
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0 || byte_offset + slot_size > fill_size_bytes) {
        return false;
      }

      const auto fill_value = lower_repeated_byte_initializer_value(slot_type_it->second, fill_byte);
      if (!fill_value.has_value()) {
        return false;
      }
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = slot_name,
          .value = *fill_value,
      });
      covered_bytes += slot_size;
    }
    return covered_bytes == fill_size_bytes;
  };

  if (const auto aggregate_it = local_aggregate_slots.find(std::string(dst_operand));
      aggregate_it != local_aggregate_slots.end()) {
    const auto aggregate_layout =
        lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
    if (!aggregate_layout.has_value()) {
      return false;
    }
    return fill_leaf_slots(collect_sorted_leaf_slots_for_memops(aggregate_it->second),
                           aggregate_layout->size_bytes);
  }

  const auto array_view = resolve_local_array_view(dst_operand);
  if (array_view.has_value()) {
    if (array_view->base_index > array_view->element_slots.size()) {
      return false;
    }

    const auto element_size = type_size_bytes(array_view->element_type);
    if (element_size == 0) {
      return false;
    }

    const auto target_count = array_view->element_slots.size() - array_view->base_index;
    std::vector<std::pair<std::size_t, std::string>> leaf_slots;
    leaf_slots.reserve(target_count);
    for (std::size_t index = 0; index < target_count; ++index) {
      leaf_slots.emplace_back(index * element_size,
                              array_view->element_slots[array_view->base_index + index]);
    }
    return fill_leaf_slots(leaf_slots, target_count * element_size);
  }

  const auto scalar_slot = resolve_local_memset_scalar_slot(dst_operand);
  if (!scalar_slot.has_value()) {
    return false;
  }

  std::vector<std::pair<std::size_t, std::string>> leaf_slots;
  leaf_slots.emplace_back(0, scalar_slot->slot_name);
  return fill_leaf_slots(leaf_slots, scalar_slot->size_bytes);
}

bool BirFunctionLowerer::try_lower_immediate_local_memcpy(
    std::string_view dst_operand,
    std::string_view src_operand,
    std::size_t requested_size,
    const bir::Function& lowered_function,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    const LocalPointerSlots& local_pointer_slots,
    const LocalArraySlotMap& local_array_slots,
    const LocalPointerArrayBaseMap& local_pointer_array_bases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    std::vector<bir::Inst>* lowered_insts) {
  const auto build_memcpy_leaf_view_from_aggregate =
      [&](const LocalAggregateSlots& aggregate_slots) -> std::optional<LocalMemcpyLeafView> {
    const auto aggregate_layout =
        lower_byval_aggregate_layout(aggregate_slots.type_text, type_decls);
    if (!aggregate_layout.has_value()) {
      return std::nullopt;
    }
    LocalMemcpyLeafView view{
        .size_bytes = aggregate_layout->size_bytes,
    };
    std::vector<std::pair<std::size_t, std::string>> leaves;
    leaves.reserve(aggregate_slots.leaf_slots.size());
    const auto begin_offset = aggregate_slots.base_byte_offset;
    const auto end_offset = begin_offset + aggregate_layout->size_bytes;
    for (const auto& [byte_offset, slot_name] : aggregate_slots.leaf_slots) {
      if (byte_offset < begin_offset || byte_offset >= end_offset) {
        continue;
      }
      leaves.push_back({byte_offset - begin_offset, slot_name});
    }
    std::sort(leaves.begin(),
              leaves.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    view.leaves.reserve(leaves.size());
    for (const auto& [byte_offset, slot_name] : leaves) {
      const auto slot_type_it = local_slot_types.find(slot_name);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }
      view.leaves.push_back(LocalMemcpyLeaf{
          .byte_offset = byte_offset,
          .type = slot_type_it->second,
          .slot_name = slot_name,
      });
    }
    return view;
  };
  const auto resolve_local_array_view =
      [&](std::string_view operand) -> std::optional<LocalArrayView> {
    if (const auto array_it = local_array_slots.find(std::string(operand));
        array_it != local_array_slots.end()) {
      return LocalArrayView{
          .element_type = array_it->second.element_type,
          .element_slots = array_it->second.element_slots,
          .base_index = 0,
      };
    }
    if (const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
        array_base_it != local_pointer_array_bases.end() &&
        array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
      const auto slot_type_it =
          local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
      if (slot_type_it != local_slot_types.end()) {
        return LocalArrayView{
            .element_type = slot_type_it->second,
            .element_slots = array_base_it->second.element_slots,
            .base_index = array_base_it->second.base_index,
        };
      }
    }

    const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
    if (ptr_slot_it == local_pointer_slots.end()) {
      return std::nullopt;
    }
    const auto dot = ptr_slot_it->second.rfind('.');
    if (dot == std::string::npos) {
      return std::nullopt;
    }

    const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
    if (slot_type_it == local_slot_types.end()) {
      return std::nullopt;
    }

    const std::string base_name = ptr_slot_it->second.substr(0, dot);
    const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
    if (!base_offset.has_value() || *base_offset < 0) {
      return std::nullopt;
    }

    if (const auto base_array_it = local_array_slots.find(base_name);
        base_array_it != local_array_slots.end()) {
      const auto base_index = static_cast<std::size_t>(*base_offset);
      if (base_index >= base_array_it->second.element_slots.size() ||
          base_array_it->second.element_type != slot_type_it->second) {
        return std::nullopt;
      }
      return LocalArrayView{
          .element_type = slot_type_it->second,
          .element_slots = base_array_it->second.element_slots,
          .base_index = base_index,
      };
    }

    const auto base_aggregate_it = local_aggregate_slots.find(base_name);
    if (base_aggregate_it == local_aggregate_slots.end()) {
      return std::nullopt;
    }

    const auto extent = find_repeated_aggregate_extent_at_offset(
        base_aggregate_it->second.storage_type_text,
        static_cast<std::size_t>(*base_offset),
        render_type(slot_type_it->second),
        type_decls);
    if (!extent.has_value()) {
      return std::nullopt;
    }

    std::vector<std::string> element_slots;
    element_slots.reserve(extent->element_count);
    for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
      const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
          static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
      if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
        return std::nullopt;
      }
      element_slots.push_back(leaf_it->second);
    }

    return LocalArrayView{
        .element_type = slot_type_it->second,
        .element_slots = std::move(element_slots),
        .base_index = 0,
    };
  };
  const auto resolve_local_memcpy_leaf_view =
      [&](std::string_view operand) -> std::optional<LocalMemcpyLeafView> {
    if (const auto aggregate_it = local_aggregate_slots.find(std::string(operand));
        aggregate_it != local_aggregate_slots.end()) {
      return build_memcpy_leaf_view_from_aggregate(aggregate_it->second);
    }

    if (const auto ptr_it = local_pointer_slots.find(std::string(operand));
        ptr_it != local_pointer_slots.end()) {
      const LocalAggregateSlots* best_match = nullptr;
      std::size_t best_size = 0;
      for (const auto& [aggregate_name, aggregate_slots] : local_aggregate_slots) {
        (void)aggregate_name;
        const auto base_leaf_it = aggregate_slots.leaf_slots.find(aggregate_slots.base_byte_offset);
        if (base_leaf_it == aggregate_slots.leaf_slots.end() ||
            base_leaf_it->second != ptr_it->second) {
          continue;
        }
        const auto aggregate_layout =
            lower_byval_aggregate_layout(aggregate_slots.type_text, type_decls);
        if (!aggregate_layout.has_value()) {
          continue;
        }
        if (best_match == nullptr || aggregate_layout->size_bytes < best_size) {
          best_match = &aggregate_slots;
          best_size = aggregate_layout->size_bytes;
        }
      }
      if (best_match != nullptr) {
        return build_memcpy_leaf_view_from_aggregate(*best_match);
      }
    }

    const auto array_view = resolve_local_array_view(operand);
    if (!array_view.has_value() || array_view->base_index > array_view->element_slots.size()) {
      return std::nullopt;
    }

    const auto element_size = type_size_bytes(array_view->element_type);
    if (element_size == 0) {
      return std::nullopt;
    }

    const auto count = array_view->element_slots.size() - array_view->base_index;
    LocalMemcpyLeafView view{
        .size_bytes = count * element_size,
    };
    view.leaves.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
      const auto& slot_name = array_view->element_slots[array_view->base_index + index];
      const auto slot_type_it = local_slot_types.find(slot_name);
      if (slot_type_it == local_slot_types.end() || slot_type_it->second != array_view->element_type) {
        return std::nullopt;
      }
      view.leaves.push_back(LocalMemcpyLeaf{
          .byte_offset = index * element_size,
          .type = array_view->element_type,
          .slot_name = slot_name,
      });
    }
    return view;
  };
  const auto resolve_local_memcpy_scalar_slot =
      [&](std::string_view operand) -> std::optional<LocalMemcpyScalarSlot> {
    const auto ptr_it = local_pointer_slots.find(std::string(operand));
    if (ptr_it == local_pointer_slots.end()) {
      return std::nullopt;
    }

    const auto slot_type_it = local_slot_types.find(ptr_it->second);
    if (slot_type_it == local_slot_types.end()) {
      return std::nullopt;
    }

    const auto slot_size = type_size_bytes(slot_type_it->second);
    if (slot_size == 0) {
      return std::nullopt;
    }

    auto slot_align = slot_size;
    if (const auto slot_it =
            std::find_if(lowered_function.local_slots.begin(),
                         lowered_function.local_slots.end(),
                         [&](const bir::LocalSlot& slot) { return slot.name == ptr_it->second; });
        slot_it != lowered_function.local_slots.end() && slot_it->align_bytes != 0) {
      slot_align = slot_it->align_bytes;
    }

    return LocalMemcpyScalarSlot{
        .slot_name = ptr_it->second,
        .type = slot_type_it->second,
        .size_bytes = slot_size,
        .align_bytes = slot_align,
    };
  };
  const auto append_local_leaf_copy = [&](const LocalMemcpyLeafView& source_view,
                                          const LocalMemcpyLeafView& target_view) -> bool {
    const auto collect_requested_prefix =
        [&](const LocalMemcpyLeafView& view) -> std::optional<std::vector<LocalMemcpyLeaf>> {
      if (requested_size > view.size_bytes) {
        return std::nullopt;
      }

      std::vector<LocalMemcpyLeaf> prefix;
      prefix.reserve(view.leaves.size());
      std::size_t covered_bytes = 0;
      for (const auto& leaf : view.leaves) {
        if (covered_bytes == requested_size) {
          break;
        }
        if (leaf.byte_offset != covered_bytes) {
          return std::nullopt;
        }

        const auto leaf_size = type_size_bytes(leaf.type);
        if (leaf_size == 0 || leaf.byte_offset + leaf_size > requested_size) {
          return std::nullopt;
        }

        prefix.push_back(leaf);
        covered_bytes += leaf_size;
      }

      if (covered_bytes != requested_size) {
        return std::nullopt;
      }
      return prefix;
    };

    const auto source_prefix = collect_requested_prefix(source_view);
    const auto target_prefix = collect_requested_prefix(target_view);
    if (!source_prefix.has_value() || !target_prefix.has_value() ||
        source_prefix->size() != target_prefix->size()) {
      return false;
    }

    for (std::size_t index = 0; index < target_prefix->size(); ++index) {
      const auto& source_leaf = (*source_prefix)[index];
      const auto& target_leaf = (*target_prefix)[index];
      if (source_leaf.byte_offset != target_leaf.byte_offset || source_leaf.type != target_leaf.type) {
        return false;
      }
      const std::string copy_name =
          std::string(dst_operand) + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(target_leaf.type, copy_name),
          .slot_name = source_leaf.slot_name,
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = target_leaf.slot_name,
          .value = bir::Value::named(target_leaf.type, copy_name),
      });
    }
    return true;
  };

  const auto append_leaf_view_to_scalar_slot = [&](const LocalMemcpyLeafView& source_view,
                                                    const LocalMemcpyScalarSlot& target_slot) -> bool {
    if (requested_size > target_slot.size_bytes) {
      return false;
    }

    std::size_t covered_bytes = 0;
    for (const auto& source_leaf : source_view.leaves) {
      if (covered_bytes == requested_size) {
        break;
      }
      const auto leaf_size = type_size_bytes(source_leaf.type);
      if (leaf_size == 0 || source_leaf.byte_offset != covered_bytes ||
          source_leaf.byte_offset + leaf_size > requested_size) {
        return false;
      }

      const std::string copy_name =
          target_slot.slot_name + ".memcpy.copy." + std::to_string(source_leaf.byte_offset);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(source_leaf.type, copy_name),
          .slot_name = source_leaf.slot_name,
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = source_leaf.slot_name,
          .value = bir::Value::named(source_leaf.type, copy_name),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = target_slot.slot_name,
                  .byte_offset = static_cast<std::int64_t>(source_leaf.byte_offset),
                  .size_bytes = leaf_size,
                  .align_bytes = std::min(target_slot.align_bytes, leaf_size),
              },
      });
      covered_bytes += leaf_size;
    }
    return covered_bytes == requested_size;
  };

  const auto append_scalar_slot_to_leaf_view = [&](const LocalMemcpyScalarSlot& source_slot,
                                                    const LocalMemcpyLeafView& target_view) -> bool {
    if (requested_size > source_slot.size_bytes) {
      return false;
    }

    std::size_t covered_bytes = 0;
    for (const auto& target_leaf : target_view.leaves) {
      if (covered_bytes == requested_size) {
        break;
      }
      const auto leaf_size = type_size_bytes(target_leaf.type);
      if (leaf_size == 0 || target_leaf.byte_offset != covered_bytes ||
          target_leaf.byte_offset + leaf_size > requested_size) {
        return false;
      }

      const std::string copy_name =
          target_leaf.slot_name + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(target_leaf.type, copy_name),
          .slot_name = target_leaf.slot_name,
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = source_slot.slot_name,
                  .byte_offset = static_cast<std::int64_t>(target_leaf.byte_offset),
                  .size_bytes = leaf_size,
                  .align_bytes = std::min(source_slot.align_bytes, leaf_size),
              },
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = target_leaf.slot_name,
          .value = bir::Value::named(target_leaf.type, copy_name),
      });
      covered_bytes += leaf_size;
    }
    return covered_bytes == requested_size;
  };
  const auto append_pointer_value_to_leaf_view = [&](std::string_view source_pointer,
                                                     const LocalMemcpyLeafView& target_view) -> bool {
    std::size_t covered_bytes = 0;
    for (const auto& target_leaf : target_view.leaves) {
      if (covered_bytes == requested_size) {
        break;
      }
      const auto leaf_size = type_size_bytes(target_leaf.type);
      if (leaf_size == 0 || target_leaf.byte_offset != covered_bytes ||
          target_leaf.byte_offset + leaf_size > requested_size) {
        return false;
      }

      const std::string copy_name =
          target_leaf.slot_name + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(target_leaf.type, copy_name),
          .slot_name = target_leaf.slot_name,
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value = bir::Value::named(bir::TypeKind::Ptr, std::string(source_pointer)),
                  .byte_offset = static_cast<std::int64_t>(target_leaf.byte_offset),
                  .size_bytes = leaf_size,
                  .align_bytes = leaf_size,
              },
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = target_leaf.slot_name,
          .value = bir::Value::named(target_leaf.type, copy_name),
      });
      covered_bytes += leaf_size;
    }
    return covered_bytes == requested_size;
  };
  const auto append_pointer_value_to_scalar_slot = [&](std::string_view source_pointer,
                                                        const LocalMemcpyScalarSlot& target_slot) -> bool {
    if (requested_size > target_slot.size_bytes) {
      return false;
    }
    std::size_t copied_bytes = 0;
    while (copied_bytes < requested_size) {
      const std::size_t remaining_bytes = requested_size - copied_bytes;
      const std::size_t chunk_size = remaining_bytes >= 4 ? 4 : 1;
      const auto chunk_type = chunk_size == 4 ? bir::TypeKind::I32 : bir::TypeKind::I8;
      const std::string copy_name =
          target_slot.slot_name + ".memcpy.copy." + std::to_string(copied_bytes);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(chunk_type, copy_name),
          .slot_name = target_slot.slot_name,
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value = bir::Value::named(bir::TypeKind::Ptr, std::string(source_pointer)),
                  .byte_offset = static_cast<std::int64_t>(copied_bytes),
                  .size_bytes = chunk_size,
                  .align_bytes = std::min(target_slot.align_bytes, chunk_size),
              },
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = target_slot.slot_name,
          .value = bir::Value::named(chunk_type, copy_name),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = target_slot.slot_name,
                  .byte_offset = static_cast<std::int64_t>(copied_bytes),
                  .size_bytes = chunk_size,
                  .align_bytes = std::min(target_slot.align_bytes, chunk_size),
              },
      });
      copied_bytes += chunk_size;
    }
    return true;
  };

  const auto target_view = resolve_local_memcpy_leaf_view(dst_operand);
  if (target_view.has_value()) {
    const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
    if (source_view.has_value() && append_local_leaf_copy(*source_view, *target_view)) {
      return true;
    }
    const auto source_scalar_slot = resolve_local_memcpy_scalar_slot(src_operand);
    if (source_scalar_slot.has_value() &&
        append_scalar_slot_to_leaf_view(*source_scalar_slot, *target_view)) {
      return true;
    }
    return local_pointer_slots.find(std::string(src_operand)) == local_pointer_slots.end() &&
           append_pointer_value_to_leaf_view(src_operand, *target_view);
  }

  if (const auto target_scalar_slot = resolve_local_memcpy_scalar_slot(dst_operand);
      target_scalar_slot.has_value()) {
    const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
    if (source_view.has_value() &&
        append_leaf_view_to_scalar_slot(*source_view, *target_scalar_slot)) {
      return true;
    }
    return local_pointer_slots.find(std::string(src_operand)) == local_pointer_slots.end() &&
           append_pointer_value_to_scalar_slot(src_operand, *target_scalar_slot);
  }

  return false;
}

std::optional<std::string> BirFunctionLowerer::resolve_local_aggregate_gep_slot(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  if (const auto raw_byte_slice_leaf = resolve_local_aggregate_raw_byte_slice_leaf(
          base_type_text, gep, value_aliases, type_decls, aggregate_slots);
      raw_byte_slice_leaf.has_value()) {
    const auto slot_it = aggregate_slots.leaf_slots.find(raw_byte_slice_leaf->byte_offset);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    return slot_it->second;
  }

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
  if (const auto raw_byte_slice_leaf = resolve_local_aggregate_raw_byte_slice_leaf(
          base_type_text, gep, value_aliases, type_decls, aggregate_slots);
      raw_byte_slice_leaf.has_value()) {
    return LocalAggregateGepTarget{
        .type_text = raw_byte_slice_leaf->type_text,
        .byte_offset = static_cast<std::int64_t>(raw_byte_slice_leaf->byte_offset),
    };
  }

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

std::optional<bool> BirFunctionLowerer::try_lower_dynamic_local_aggregate_gep_projection(
    const c4c::codegen::lir::LirGepOp& gep,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays) {
  const auto dynamic_local_aggregate_it =
      dynamic_local_aggregate_arrays.find(std::string(gep.ptr.str()));
  if (dynamic_local_aggregate_it == dynamic_local_aggregate_arrays.end()) {
    return std::nullopt;
  }

  const auto projection = resolve_dynamic_local_aggregate_gep_projection(
      gep, value_aliases, type_decls, dynamic_local_aggregate_it->second);
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
  const auto local_slot_ptr_it = local_slot_pointer_values->find(std::string(gep.ptr.str()));
  if (local_slot_ptr_it == local_slot_pointer_values->end()) {
    return std::nullopt;
  }

  auto resolved_target = resolve_relative_gep_target(local_slot_ptr_it->second.type_text,
                                                     local_slot_ptr_it->second.byte_offset,
                                                     gep,
                                                     value_aliases,
                                                     type_decls);
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

  const auto leaf_layout = compute_aggregate_type_layout(resolved_target->type_text, type_decls);
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
            type_decls);
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

bool BirFunctionLowerer::try_lower_local_slot_pointer_store(
    const LocalSlotAddress& local_slot_ptr,
    bir::TypeKind value_type,
    const bir::Value& value,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  const auto direct_slot_type_it = local_slot_types.find(local_slot_ptr.slot_name);
  const bool exact_local_slot_type = local_slot_ptr.value_type == value_type;
  const bool can_use_direct_local_slot =
      direct_slot_type_it != local_slot_types.end() && direct_slot_type_it->second == value_type;
  if (exact_local_slot_type && can_use_direct_local_slot && local_slot_ptr.byte_offset == 0) {
    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = local_slot_ptr.slot_name,
        .value = value,
    });
    return true;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return false;
  }

  lowered_insts->push_back(bir::StoreLocalInst{
      .slot_name = local_slot_ptr.slot_name,
      .value = value,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = local_slot_ptr.slot_name,
              .byte_offset = static_cast<std::int64_t>(local_slot_ptr.byte_offset),
              .size_bytes = slot_size,
              .align_bytes = slot_size,
          },
  });
  return true;
}

bool BirFunctionLowerer::try_lower_local_slot_pointer_load(
    std::string_view result_name,
    const LocalSlotAddress& local_slot_ptr,
    bir::TypeKind value_type,
    const LocalSlotTypes& local_slot_types,
    const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
    const LocalAddressSlots& local_address_slots,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    ValueMap* value_aliases,
    LocalSlotPointerValues* local_slot_pointer_values,
    GlobalPointerMap* global_pointer_slots,
    std::vector<bir::Inst>* lowered_insts) {
  const auto result = std::string(result_name);
  const auto direct_slot_type_it = local_slot_types.find(local_slot_ptr.slot_name);
  const bool exact_local_slot_type = local_slot_ptr.value_type == value_type;
  const bool can_use_direct_local_slot =
      direct_slot_type_it != local_slot_types.end() && direct_slot_type_it->second == value_type;

  if (value_type == bir::TypeKind::Ptr && exact_local_slot_type && can_use_direct_local_slot &&
      local_slot_ptr.byte_offset == 0) {
    if (const auto nested_local_slot_it = local_slot_address_slots.find(local_slot_ptr.slot_name);
        nested_local_slot_it != local_slot_address_slots.end()) {
      (*local_slot_pointer_values)[result] = nested_local_slot_it->second;
    }
    if (const auto nested_global_it = local_address_slots.find(local_slot_ptr.slot_name);
        nested_global_it != local_address_slots.end()) {
      const bool preserve_loaded_pointer_provenance =
          local_indirect_pointer_slots.find(local_slot_ptr.slot_name) !=
          local_indirect_pointer_slots.end();
      if (!preserve_loaded_pointer_provenance) {
        if (nested_global_it->second.byte_offset == 0 &&
            is_known_function_symbol(nested_global_it->second.global_name, function_symbols)) {
          (*value_aliases)[result] =
              bir::Value::named(bir::TypeKind::Ptr, "@" + nested_global_it->second.global_name);
          (*global_pointer_slots)[result] = nested_global_it->second;
        } else if (const auto honest_base =
                       resolve_honest_pointer_base(nested_global_it->second, global_types);
                   honest_base.has_value() && honest_base->byte_offset == 0) {
          (*value_aliases)[result] =
              bir::Value::named(bir::TypeKind::Ptr, "@" + honest_base->global_name);
          (*global_pointer_slots)[result] = *honest_base;
        }
      }
      if (global_pointer_slots->find(result) == global_pointer_slots->end()) {
        (*global_pointer_slots)[result] = nested_global_it->second;
      }
    }
  }

  if (exact_local_slot_type && can_use_direct_local_slot && local_slot_ptr.byte_offset == 0) {
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(value_type, result),
        .slot_name = local_slot_ptr.slot_name,
    });
    return true;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return false;
  }
  const std::string scratch_slot = result + ".addr";
  if (!ensure_local_scratch_slot(scratch_slot, value_type, slot_size)) {
    return false;
  }
  lowered_insts->push_back(bir::LoadLocalInst{
      .result = bir::Value::named(value_type, result),
      .slot_name = scratch_slot,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = local_slot_ptr.slot_name,
              .byte_offset = static_cast<std::int64_t>(local_slot_ptr.byte_offset),
              .size_bytes = slot_size,
              .align_bytes = slot_size,
          },
  });
  return true;
}

BirFunctionLowerer::LocalSlotStoreResult BirFunctionLowerer::try_lower_local_slot_store(
    std::string_view ptr_name,
    const c4c::codegen::lir::LirOperand& stored_operand,
    bir::TypeKind value_type,
    const bir::Value& value,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    const LocalPointerSlots& local_pointer_slots,
    const LocalSlotTypes& local_slot_types,
    const LocalAggregateFieldSet& local_aggregate_field_slots,
    const LocalArraySlotMap& local_array_slots,
    const LocalAggregateSlotMap& local_aggregate_slots,
    const LocalPointerArrayBaseMap& local_pointer_array_bases,
    const LocalSlotPointerValues& local_slot_pointer_values,
    const PointerAddressMap& pointer_value_addresses,
    const GlobalPointerMap& global_pointer_slots,
    const GlobalAddressIntMap& global_address_ints,
    LocalPointerValueAliasMap* local_pointer_value_aliases,
    LocalIndirectPointerSlotSet* local_indirect_pointer_slots,
    PointerAddressMap* local_pointer_slot_addresses,
    LocalSlotAddressSlots* local_slot_address_slots,
    LocalAddressSlots* local_address_slots,
    std::vector<bir::Inst>* lowered_insts) {
  const auto ptr_it = local_pointer_slots.find(std::string(ptr_name));
  if (ptr_it == local_pointer_slots.end()) {
    return LocalSlotStoreResult::NotHandled;
  }

  const auto slot_it = local_slot_types.find(ptr_it->second);
  if (slot_it == local_slot_types.end() || slot_it->second != value_type) {
    return LocalSlotStoreResult::Failed;
  }

  const bool tracks_pointer_value_slot =
      local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
      is_local_array_element_slot(ptr_it->second, local_array_slots);
  if (tracks_pointer_value_slot) {
    if (value_type == bir::TypeKind::Ptr) {
      const auto pointer_alias = resolve_local_aggregate_pointer_value_alias(
          stored_operand, value_aliases, local_aggregate_slots, function_symbols);
      if (pointer_alias.has_value()) {
        (*local_pointer_value_aliases)[ptr_it->second] = *pointer_alias;
      } else {
        local_pointer_value_aliases->erase(ptr_it->second);
      }
    } else {
      local_pointer_value_aliases->erase(ptr_it->second);
    }
  }

  if (value_type == bir::TypeKind::I64) {
    local_pointer_slot_addresses->erase(ptr_it->second);
    local_indirect_pointer_slots->erase(ptr_it->second);
    if (stored_operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto global_addr_it = global_address_ints.find(stored_operand.str());
      if (global_addr_it != global_address_ints.end()) {
        local_slot_address_slots->erase(ptr_it->second);
        (*local_address_slots)[ptr_it->second] = global_addr_it->second;
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = ptr_it->second,
            .value = value,
        });
        return LocalSlotStoreResult::Lowered;
      }
    }
    local_slot_address_slots->erase(ptr_it->second);
    local_address_slots->erase(ptr_it->second);
  } else if (value_type == bir::TypeKind::Ptr) {
    bool stored_pointer_value_address = false;
    bool stored_local_slot_address = false;
    if (stored_operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = stored_operand.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end()) {
        if (!is_known_function_symbol(global_name, function_symbols)) {
          return LocalSlotStoreResult::Failed;
        }
        local_pointer_slot_addresses->erase(ptr_it->second);
        local_slot_address_slots->erase(ptr_it->second);
        (*local_address_slots)[ptr_it->second] = GlobalAddress{
            .global_name = global_name,
            .value_type = bir::TypeKind::Ptr,
            .byte_offset = 0,
        };
        local_indirect_pointer_slots->erase(ptr_it->second);
        if (!tracks_pointer_value_slot) {
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = ptr_it->second,
              .value = value,
          });
          return LocalSlotStoreResult::Lowered;
        }
      }
      if (global_it == global_types.end()) {
        stored_local_slot_address = false;
      } else if (!global_it->second.supports_linear_addressing) {
        return LocalSlotStoreResult::Failed;
      } else {
        local_pointer_slot_addresses->erase(ptr_it->second);
        local_slot_address_slots->erase(ptr_it->second);
        (*local_address_slots)[ptr_it->second] = GlobalAddress{
            .global_name = global_name,
            .value_type = global_it->second.value_type,
            .byte_offset = 0,
        };
        local_indirect_pointer_slots->erase(ptr_it->second);
        if (!tracks_pointer_value_slot) {
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = ptr_it->second,
              .value = value,
          });
          return LocalSlotStoreResult::Lowered;
        }
      }
    }
    if (stored_operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto local_slot_ptr_val_it = local_slot_pointer_values.find(stored_operand.str());
      if (local_slot_ptr_val_it != local_slot_pointer_values.end()) {
        local_pointer_slot_addresses->erase(ptr_it->second);
        auto stored_address = local_slot_ptr_val_it->second;
        if (stored_address.array_element_slots.empty()) {
          if (const auto array_base_it = local_pointer_array_bases.find(stored_operand.str());
              array_base_it != local_pointer_array_bases.end()) {
            stored_address.array_element_slots = array_base_it->second.element_slots;
            stored_address.array_base_index = array_base_it->second.base_index;
          } else if (const auto local_aggregate_it = local_aggregate_slots.find(stored_operand.str());
                     local_aggregate_it != local_aggregate_slots.end()) {
            if (const auto array_slots = collect_local_scalar_array_slots(
                    local_aggregate_it->second.type_text, type_decls, local_aggregate_it->second);
                array_slots.has_value()) {
              stored_address.array_element_slots = *array_slots;
            }
          }
        }
        (*local_slot_address_slots)[ptr_it->second] = std::move(stored_address);
        local_address_slots->erase(ptr_it->second);
        local_indirect_pointer_slots->erase(ptr_it->second);
        stored_local_slot_address = true;
      }
      if (const auto pointer_value_it = pointer_value_addresses.find(stored_operand.str());
          pointer_value_it != pointer_value_addresses.end()) {
        (*local_pointer_slot_addresses)[ptr_it->second] = pointer_value_it->second;
        local_slot_address_slots->erase(ptr_it->second);
        local_address_slots->erase(ptr_it->second);
        local_indirect_pointer_slots->erase(ptr_it->second);
        stored_pointer_value_address = true;
      }
      const auto local_ptr_it = local_pointer_slots.find(stored_operand.str());
      if (local_ptr_it != local_pointer_slots.end() &&
          local_slot_ptr_val_it == local_slot_pointer_values.end()) {
        const auto source_slot_it = local_slot_types.find(local_ptr_it->second);
        if (source_slot_it == local_slot_types.end()) {
          return LocalSlotStoreResult::Failed;
        }
        local_pointer_slot_addresses->erase(ptr_it->second);
        LocalSlotAddress stored_address{
            .slot_name = local_ptr_it->second,
            .value_type = source_slot_it->second,
            .byte_offset = 0,
            .storage_type_text = render_type(source_slot_it->second),
            .type_text = render_type(source_slot_it->second),
        };
        if (const auto array_base_it = local_pointer_array_bases.find(stored_operand.str());
            array_base_it != local_pointer_array_bases.end()) {
          const auto slot_size = type_size_bytes(source_slot_it->second);
          if (slot_size == 0 || array_base_it->second.element_slots.empty() ||
              array_base_it->second.base_index >= array_base_it->second.element_slots.size()) {
            return LocalSlotStoreResult::Failed;
          }
          stored_address.slot_name = array_base_it->second.element_slots.front();
          stored_address.byte_offset = array_base_it->second.base_index * slot_size;
          stored_address.array_element_slots = array_base_it->second.element_slots;
          stored_address.array_base_index = array_base_it->second.base_index;
        }
        (*local_slot_address_slots)[ptr_it->second] = std::move(stored_address);
        local_address_slots->erase(ptr_it->second);
        local_indirect_pointer_slots->erase(ptr_it->second);
        stored_local_slot_address = true;
      } else if (const auto local_aggregate_it = local_aggregate_slots.find(stored_operand.str());
                 local_aggregate_it != local_aggregate_slots.end() &&
                 local_slot_ptr_val_it == local_slot_pointer_values.end()) {
        const auto leaf_it =
            local_aggregate_it->second.leaf_slots.find(local_aggregate_it->second.base_byte_offset);
        if (leaf_it == local_aggregate_it->second.leaf_slots.end()) {
          return LocalSlotStoreResult::Failed;
        }
        local_pointer_slot_addresses->erase(ptr_it->second);
        const auto target_layout =
            compute_aggregate_type_layout(local_aggregate_it->second.type_text, type_decls);
        auto stored_address = LocalSlotAddress{
            .slot_name = leaf_it->second,
            .value_type = target_layout.kind == AggregateTypeLayout::Kind::Scalar
                              ? target_layout.scalar_type
                              : bir::TypeKind::Void,
            .byte_offset = 0,
            .storage_type_text = local_aggregate_it->second.storage_type_text,
            .type_text = local_aggregate_it->second.type_text,
        };
        if (const auto array_slots = collect_local_scalar_array_slots(
                local_aggregate_it->second.type_text, type_decls, local_aggregate_it->second);
            array_slots.has_value()) {
          stored_address.array_element_slots = *array_slots;
        }
        (*local_slot_address_slots)[ptr_it->second] = std::move(stored_address);
        local_address_slots->erase(ptr_it->second);
        local_indirect_pointer_slots->erase(ptr_it->second);
        stored_local_slot_address = true;
      }
      const auto global_ptr_it = global_pointer_slots.find(stored_operand.str());
      if (global_ptr_it != global_pointer_slots.end()) {
        local_pointer_slot_addresses->erase(ptr_it->second);
        local_slot_address_slots->erase(ptr_it->second);
        (*local_address_slots)[ptr_it->second] = global_ptr_it->second;
        local_indirect_pointer_slots->insert(ptr_it->second);
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = ptr_it->second,
            .value = value,
        });
        return LocalSlotStoreResult::Lowered;
      }
    }
    if (!stored_local_slot_address && !stored_pointer_value_address) {
      local_pointer_slot_addresses->erase(ptr_it->second);
      local_slot_address_slots->erase(ptr_it->second);
      local_address_slots->erase(ptr_it->second);
      local_indirect_pointer_slots->erase(ptr_it->second);
    }
  }

  lowered_insts->push_back(bir::StoreLocalInst{
      .slot_name = ptr_it->second,
      .value = value,
  });
  return LocalSlotStoreResult::Lowered;
}

bool BirFunctionLowerer::try_lower_nonpointer_local_slot_load(
    std::string_view result_name,
    std::string_view slot_name,
    bir::TypeKind value_type,
    const LocalAddressSlots& local_address_slots,
    GlobalAddressIntMap* global_address_ints,
    std::vector<bir::Inst>* lowered_insts) {
  const auto result = std::string(result_name);
  const auto slot = std::string(slot_name);

  if (value_type == bir::TypeKind::I64) {
    if (const auto addr_it = local_address_slots.find(slot); addr_it != local_address_slots.end()) {
      (*global_address_ints)[result] = addr_it->second;
      return true;
    }
  }

  lowered_insts->push_back(bir::LoadLocalInst{
      .result = bir::Value::named(value_type, result),
      .slot_name = slot,
  });
  return true;
}

BirFunctionLowerer::LocalSlotLoadResult BirFunctionLowerer::try_lower_local_slot_load(
    std::string_view result_name,
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const LocalPointerSlots& local_pointer_slots,
    const LocalSlotTypes& local_slot_types,
    const LocalAggregateFieldSet& local_aggregate_field_slots,
    const LocalArraySlotMap& local_array_slots,
    const LocalPointerValueAliasMap& local_pointer_value_aliases,
    const TypeDeclMap& type_decls,
    const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
    const LocalAddressSlots& local_address_slots,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const PointerAddressMap& local_pointer_slot_addresses,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    ValueMap* value_aliases,
    LocalSlotPointerValues* local_slot_pointer_values,
    LocalAggregateSlotMap* local_aggregate_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    GlobalPointerMap* global_pointer_slots,
    PointerAddressMap* pointer_value_addresses,
    GlobalAddressIntMap* global_address_ints,
    std::vector<bir::Inst>* lowered_insts) {
  const auto ptr_it = local_pointer_slots.find(std::string(ptr_name));
  if (ptr_it == local_pointer_slots.end()) {
    return LocalSlotLoadResult::NotHandled;
  }

  const auto slot_it = local_slot_types.find(ptr_it->second);
  if (slot_it == local_slot_types.end() || slot_it->second != value_type) {
    return LocalSlotLoadResult::Failed;
  }

  if (value_type == bir::TypeKind::Ptr) {
    return try_lower_tracked_local_pointer_slot_load(result_name,
                                                     ptr_it->second,
                                                     local_aggregate_field_slots,
                                                     local_array_slots,
                                                     local_pointer_value_aliases,
                                                     type_decls,
                                                     local_indirect_pointer_slots,
                                                     local_address_slots,
                                                     local_slot_address_slots,
                                                     local_pointer_slot_addresses,
                                                     global_types,
                                                     function_symbols,
                                                     value_aliases,
                                                     local_slot_pointer_values,
                                                     local_aggregate_slots,
                                                     local_pointer_array_bases,
                                                     global_pointer_slots,
                                                     pointer_value_addresses,
                                                     lowered_insts)
               ? LocalSlotLoadResult::Lowered
               : LocalSlotLoadResult::Failed;
  }

  return try_lower_nonpointer_local_slot_load(result_name,
                                              ptr_it->second,
                                              value_type,
                                              local_address_slots,
                                              global_address_ints,
                                              lowered_insts)
             ? LocalSlotLoadResult::Lowered
             : LocalSlotLoadResult::Failed;
}

bool BirFunctionLowerer::try_lower_tracked_local_pointer_slot_load(
    std::string_view result_name,
    std::string_view slot_name,
    const LocalAggregateFieldSet& local_aggregate_field_slots,
    const LocalArraySlotMap& local_array_slots,
    const LocalPointerValueAliasMap& local_pointer_value_aliases,
    const TypeDeclMap& type_decls,
    const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
    const LocalAddressSlots& local_address_slots,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const PointerAddressMap& local_pointer_slot_addresses,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    ValueMap* value_aliases,
    LocalSlotPointerValues* local_slot_pointer_values,
    LocalAggregateSlotMap* local_aggregate_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases,
    GlobalPointerMap* global_pointer_slots,
    PointerAddressMap* pointer_value_addresses,
    std::vector<bir::Inst>* lowered_insts) {
  const auto result = std::string(result_name);
  const auto slot = std::string(slot_name);
  const bool tracks_pointer_value_slot =
      local_aggregate_field_slots.find(slot) != local_aggregate_field_slots.end() ||
      is_local_array_element_slot(slot, local_array_slots);

  if (tracks_pointer_value_slot) {
    if (const auto pointer_alias = local_pointer_value_aliases.find(slot);
        pointer_alias != local_pointer_value_aliases.end()) {
      (*value_aliases)[result] = pointer_alias->second;
      if (const auto addr_it = local_address_slots.find(slot);
          addr_it != local_address_slots.end()) {
        (*global_pointer_slots)[result] = addr_it->second;
      }
      return true;
    }
  }

  record_loaded_local_pointer_slot_state(result,
                                         slot,
                                         local_slot_address_slots,
                                         type_decls,
                                         local_slot_pointer_values,
                                         local_aggregate_slots,
                                         local_pointer_array_bases);
  if (const auto addr_it = local_address_slots.find(slot); addr_it != local_address_slots.end()) {
    const bool preserve_loaded_pointer_provenance =
        local_indirect_pointer_slots.find(slot) != local_indirect_pointer_slots.end();
    if (!preserve_loaded_pointer_provenance) {
      if (addr_it->second.byte_offset == 0 &&
          is_known_function_symbol(addr_it->second.global_name, function_symbols)) {
        (*value_aliases)[result] =
            bir::Value::named(bir::TypeKind::Ptr, "@" + addr_it->second.global_name);
        (*global_pointer_slots)[result] = addr_it->second;
        return true;
      }
      if (const auto honest_base = resolve_honest_pointer_base(addr_it->second, global_types);
          honest_base.has_value() && honest_base->byte_offset == 0) {
        (*value_aliases)[result] =
            bir::Value::named(bir::TypeKind::Ptr, "@" + honest_base->global_name);
        (*global_pointer_slots)[result] = *honest_base;
        return true;
      }
    }
    (*global_pointer_slots)[result] = addr_it->second;
  }
  if (const auto pointer_slot_it = local_pointer_slot_addresses.find(slot);
      pointer_slot_it != local_pointer_slot_addresses.end()) {
    (*pointer_value_addresses)[result] = pointer_slot_it->second;
  }

  lowered_insts->push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, result),
      .slot_name = slot,
  });
  return true;
}

void BirFunctionLowerer::record_loaded_local_pointer_slot_state(
    std::string_view result_name,
    std::string_view slot_name,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const TypeDeclMap& type_decls,
    LocalSlotPointerValues* local_slot_pointer_values,
    LocalAggregateSlotMap* local_aggregate_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases) {
  const auto local_slot_it = local_slot_address_slots.find(std::string(slot_name));
  if (local_slot_it == local_slot_address_slots.end()) {
    return;
  }

  const auto result = std::string(result_name);
  (*local_slot_pointer_values)[result] = local_slot_it->second;
  const auto loaded_layout =
      compute_aggregate_type_layout(local_slot_it->second.type_text, type_decls);
  if (loaded_layout.kind == AggregateTypeLayout::Kind::Array &&
      !local_slot_it->second.array_element_slots.empty() &&
      local_slot_it->second.byte_offset >= 0) {
    const auto element_layout =
        compute_aggregate_type_layout(loaded_layout.element_type_text, type_decls);
    if (element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
        element_layout.size_bytes != 0) {
      LocalAggregateSlots aggregate_view{
          .storage_type_text = local_slot_it->second.storage_type_text,
          .type_text = local_slot_it->second.type_text,
          .base_byte_offset = static_cast<std::size_t>(local_slot_it->second.byte_offset),
      };
      for (std::size_t index = 0; index < local_slot_it->second.array_element_slots.size();
           ++index) {
        aggregate_view.leaf_slots.emplace(index * element_layout.size_bytes,
                                          local_slot_it->second.array_element_slots[index]);
      }
      (*local_aggregate_slots)[result] = std::move(aggregate_view);
    }
  }
  if (!local_slot_it->second.array_element_slots.empty() &&
      local_slot_it->second.byte_offset >= 0 &&
      !(loaded_layout.kind == AggregateTypeLayout::Kind::Array &&
        local_slot_it->second.array_element_slots.size() > loaded_layout.array_count)) {
    (*local_pointer_array_bases)[result] = LocalPointerArrayBase{
        .element_slots = local_slot_it->second.array_element_slots,
        .base_index = local_slot_it->second.array_base_index,
    };
  }
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

bool BirFunctionLowerer::try_lower_dynamic_local_aggregate_store(
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts,
    bool* handled) {
  const auto dynamic_local_aggregate_it = dynamic_local_aggregate_arrays.find(std::string(ptr_name));
  if (dynamic_local_aggregate_it == dynamic_local_aggregate_arrays.end()) {
    *handled = false;
    return true;
  }

  *handled = true;
  return append_dynamic_local_aggregate_store(ptr_name,
                                              value_type,
                                              value,
                                              dynamic_local_aggregate_it->second,
                                              type_decls,
                                              local_slot_types,
                                              lowered_insts);
}

bool BirFunctionLowerer::try_lower_dynamic_local_aggregate_load(
    std::string_view result_name,
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    ValueMap* value_aliases,
    std::vector<bir::Inst>* lowered_insts,
    bool* handled) {
  const auto dynamic_local_aggregate_it = dynamic_local_aggregate_arrays.find(std::string(ptr_name));
  if (dynamic_local_aggregate_it == dynamic_local_aggregate_arrays.end()) {
    *handled = false;
    return true;
  }

  *handled = true;
  const auto selected_value = load_dynamic_local_aggregate_array_value(result_name,
                                                                       value_type,
                                                                       dynamic_local_aggregate_it->second,
                                                                       type_decls,
                                                                       local_slot_types,
                                                                       lowered_insts);
  if (!selected_value.has_value()) {
    return false;
  }
  if (selected_value->kind == bir::Value::Kind::Named &&
      selected_value->name == result_name) {
    return true;
  }
  (*value_aliases)[std::string(result_name)] = *selected_value;
  return true;
}

}  // namespace c4c::backend
