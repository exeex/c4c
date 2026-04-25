#include "../lowering.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::lookup_backend_aggregate_type_layout;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::type_size_bytes;
using BackendStructuredLayoutTable = lir_to_bir_detail::BackendStructuredLayoutTable;

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

}  // namespace

std::optional<BirFunctionLowerer::AggregateTypeLayout>
BirFunctionLowerer::lower_intrinsic_aggregate_layout(
    std::string_view text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  if (structured_layouts == nullptr) {
    return lower_byval_aggregate_layout(text, type_decls);
  }

  const auto layout = lookup_backend_aggregate_type_layout(text, type_decls, *structured_layouts);
  if ((layout.kind != AggregateTypeLayout::Kind::Struct &&
       layout.kind != AggregateTypeLayout::Kind::Array) ||
      layout.size_bytes == 0 || layout.align_bytes == 0) {
    return std::nullopt;
  }
  return layout;
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
  return try_lower_immediate_local_memset(dst_operand,
                                          fill_byte,
                                          fill_size_bytes,
                                          type_decls,
                                          nullptr,
                                          local_slot_types,
                                          local_pointer_slots,
                                          local_array_slots,
                                          local_pointer_array_bases,
                                          local_aggregate_slots,
                                          lowered_insts);
}

bool BirFunctionLowerer::try_lower_immediate_local_memset(
    std::string_view dst_operand,
    std::uint8_t fill_byte,
    std::size_t fill_size_bytes,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
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

    const auto extent =
        structured_layouts != nullptr
            ? find_repeated_aggregate_extent_at_offset(
                  base_aggregate_it->second.storage_type_text,
                  static_cast<std::size_t>(*base_offset),
                  render_type(slot_type_it->second),
                  type_decls,
                  *structured_layouts)
            : find_repeated_aggregate_extent_at_offset(
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
    const auto layout =
        lower_intrinsic_aggregate_layout(aggregate_slots.type_text, type_decls, structured_layouts);
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
        lower_intrinsic_aggregate_layout(aggregate_it->second.type_text,
                                         type_decls,
                                         structured_layouts);
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
  return try_lower_immediate_local_memcpy(dst_operand,
                                          src_operand,
                                          requested_size,
                                          lowered_function,
                                          type_decls,
                                          nullptr,
                                          local_slot_types,
                                          local_pointer_slots,
                                          local_array_slots,
                                          local_pointer_array_bases,
                                          local_aggregate_slots,
                                          lowered_insts);
}

bool BirFunctionLowerer::try_lower_immediate_local_memcpy(
    std::string_view dst_operand,
    std::string_view src_operand,
    std::size_t requested_size,
    const bir::Function& lowered_function,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalSlotTypes& local_slot_types,
    const LocalPointerSlots& local_pointer_slots,
    const LocalArraySlotMap& local_array_slots,
    const LocalPointerArrayBaseMap& local_pointer_array_bases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    std::vector<bir::Inst>* lowered_insts) {
  const auto build_memcpy_leaf_view_from_aggregate =
      [&](const LocalAggregateSlots& aggregate_slots) -> std::optional<LocalMemcpyLeafView> {
    const auto aggregate_layout =
        lower_intrinsic_aggregate_layout(aggregate_slots.type_text,
                                         type_decls,
                                         structured_layouts);
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

    const auto extent =
        structured_layouts != nullptr
            ? find_repeated_aggregate_extent_at_offset(
                  base_aggregate_it->second.storage_type_text,
                  static_cast<std::size_t>(*base_offset),
                  render_type(slot_type_it->second),
                  type_decls,
                  *structured_layouts)
            : find_repeated_aggregate_extent_at_offset(
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
            lower_intrinsic_aggregate_layout(aggregate_slots.type_text,
                                             type_decls,
                                             structured_layouts);
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

bool BirFunctionLowerer::lower_memory_memcpy_inst(
    const c4c::codegen::lir::LirMemcpyOp& memcpy,
    std::vector<bir::Inst>* lowered_insts) {
  if (memcpy.dst.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      memcpy.src.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      memcpy.is_volatile) {
    return false;
  }

  const auto copy_size = lower_value(memcpy.size, bir::TypeKind::I64, value_aliases_);
  if (!copy_size.has_value() || copy_size->kind != bir::Value::Kind::Immediate ||
      copy_size->immediate < 0) {
    return false;
  }

  return try_lower_immediate_local_memcpy(memcpy.dst.str(),
                                          memcpy.src.str(),
                                          static_cast<std::size_t>(copy_size->immediate),
                                          lowered_function_,
                                          type_decls_,
                                          &structured_layouts_,
                                          local_slot_types_,
                                          local_pointer_slots_,
                                          local_array_slots_,
                                          local_pointer_array_bases_,
                                          local_aggregate_slots_,
                                          lowered_insts);
}

bool BirFunctionLowerer::lower_memory_memset_inst(
    const c4c::codegen::lir::LirMemsetOp& memset,
    std::vector<bir::Inst>* lowered_insts) {
  if (memset.dst.kind() != c4c::codegen::lir::LirOperandKind::SsaValue || memset.is_volatile) {
    return false;
  }

  const auto fill_value = lower_value(memset.byte_val, bir::TypeKind::I8, value_aliases_);
  const auto fill_size = lower_value(memset.size, bir::TypeKind::I64, value_aliases_);
  if (!fill_value.has_value() || fill_value->kind != bir::Value::Kind::Immediate ||
      !fill_size.has_value() || fill_size->kind != bir::Value::Kind::Immediate ||
      fill_size->immediate < 0) {
    return false;
  }

  return try_lower_immediate_local_memset(memset.dst.str(),
                                          static_cast<std::uint8_t>(fill_value->immediate & 0xff),
                                          static_cast<std::size_t>(fill_size->immediate),
                                          type_decls_,
                                          &structured_layouts_,
                                          local_slot_types_,
                                          local_pointer_slots_,
                                          local_array_slots_,
                                          local_pointer_array_bases_,
                                          local_aggregate_slots_,
                                          lowered_insts);
}

std::optional<bool> BirFunctionLowerer::try_lower_direct_memory_intrinsic_call(
    std::string_view symbol_name,
    const ParsedTypedCall& typed_call,
    const c4c::codegen::lir::LirCallOp& call,
    const LoweredReturnInfo& return_info,
    std::vector<bir::Inst>* lowered_insts) {
  if (symbol_name == "memcpy") {
    const auto fail_memcpy_family = [&]() -> std::optional<bool> {
      note_runtime_intrinsic_family_failure("memcpy runtime family");
      return false;
    };
    if (typed_call.args.size() != 3 || typed_call.param_types.size() != 3 ||
        c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]) != "ptr" ||
        c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]) != "ptr" ||
        c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[2]) != "i64") {
      return fail_memcpy_family();
    }

    const auto copy_size =
        lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[2].operand)),
                    bir::TypeKind::I64,
                    value_aliases_);
    if (!copy_size.has_value() || copy_size->kind != bir::Value::Kind::Immediate ||
        copy_size->immediate < 0) {
      return fail_memcpy_family();
    }

    const std::string dst_operand(typed_call.args[0].operand);
    const std::string src_operand(typed_call.args[1].operand);
    if (!try_lower_immediate_local_memcpy(dst_operand,
                                          src_operand,
                                          static_cast<std::size_t>(copy_size->immediate),
                                          lowered_function_,
                                          type_decls_,
                                          &structured_layouts_,
                                          local_slot_types_,
                                          local_pointer_slots_,
                                          local_array_slots_,
                                          local_pointer_array_bases_,
                                          local_aggregate_slots_,
                                          lowered_insts)) {
      return fail_memcpy_family();
    }

    if (call.result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      if (return_info.returned_via_sret || return_info.type != bir::TypeKind::Ptr) {
        return fail_memcpy_family();
      }
      value_aliases_[call.result.str()] = bir::Value::named(bir::TypeKind::Ptr, dst_operand);
      return true;
    }
    if (return_info.type != bir::TypeKind::Void) {
      return fail_memcpy_family();
    }
    return true;
  }

  if (symbol_name == "memset") {
    const auto fail_memset_family = [&]() -> std::optional<bool> {
      note_runtime_intrinsic_family_failure("memset runtime family");
      return false;
    };
    if (typed_call.args.size() != 3 || typed_call.param_types.size() != 3 ||
        c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]) != "ptr" ||
        c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[2]) != "i64") {
      return fail_memset_family();
    }

    const auto fill_type =
        lower_integer_type(c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]));
    if (!fill_type.has_value()) {
      return fail_memset_family();
    }

    const auto fill_value =
        lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[1].operand)),
                    *fill_type,
                    value_aliases_);
    const auto fill_size =
        lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[2].operand)),
                    bir::TypeKind::I64,
                    value_aliases_);
    if (!fill_value.has_value() || fill_value->kind != bir::Value::Kind::Immediate ||
        !fill_size.has_value() || fill_size->kind != bir::Value::Kind::Immediate ||
        fill_size->immediate < 0) {
      return fail_memset_family();
    }

    const std::string dst_operand(typed_call.args[0].operand);
    if (!try_lower_immediate_local_memset(dst_operand,
                                          static_cast<std::uint8_t>(fill_value->immediate & 0xff),
                                          static_cast<std::size_t>(fill_size->immediate),
                                          type_decls_,
                                          &structured_layouts_,
                                          local_slot_types_,
                                          local_pointer_slots_,
                                          local_array_slots_,
                                          local_pointer_array_bases_,
                                          local_aggregate_slots_,
                                          lowered_insts)) {
      return fail_memset_family();
    }
    if (call.result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      if (return_info.returned_via_sret || return_info.type != bir::TypeKind::Ptr) {
        return fail_memset_family();
      }
      value_aliases_[call.result.str()] = bir::Value::named(bir::TypeKind::Ptr, dst_operand);
      return true;
    }
    if (return_info.type != bir::TypeKind::Void) {
      return fail_memset_family();
    }
    return true;
  }

  return std::nullopt;
}

}  // namespace c4c::backend
