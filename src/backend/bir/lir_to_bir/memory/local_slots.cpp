#include "../lowering.hpp"
#include "memory_helpers.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using BackendStructuredLayoutTable = lir_to_bir_detail::BackendStructuredLayoutTable;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::lookup_backend_aggregate_type_layout;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

namespace {

BirFunctionLowerer::AggregateTypeLayout lookup_scalar_byte_offset_layout(
    std::string_view type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  return structured_layouts != nullptr
             ? lookup_backend_aggregate_type_layout(type_text, type_decls, *structured_layouts)
             : compute_aggregate_type_layout(type_text, type_decls);
}

static std::optional<ScalarLayoutLeafFacts> resolve_scalar_layout_leaf_facts_at_byte_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    std::size_t base_byte_offset) {
  const auto layout = lookup_scalar_byte_offset_layout(type_text, type_decls, structured_layouts);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Scalar:
      return target_offset == 0
                 ? std::optional<ScalarLayoutLeafFacts>(ScalarLayoutLeafFacts{
                       .type_text = c4c::backend::bir::render_type(layout.scalar_type),
                       .type = layout.scalar_type,
                       .size_bytes = layout.size_bytes,
                       .byte_offset = base_byte_offset,
                   })
                 : std::nullopt;
    case BirFunctionLowerer::AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          lookup_scalar_byte_offset_layout(layout.element_type_text, type_decls, structured_layouts);
      if (element_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      return resolve_scalar_layout_leaf_facts_at_byte_offset(
          layout.element_type_text,
          nested_offset,
          type_decls,
          structured_layouts,
          base_byte_offset + element_index * element_layout.size_bytes);
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
        return resolve_scalar_layout_leaf_facts_at_byte_offset(
            layout.fields[index].type_text,
            target_offset - field_begin,
            type_decls,
            structured_layouts,
            base_byte_offset + field_begin);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<ScalarLayoutByteOffsetFacts> resolve_scalar_layout_facts_at_byte_offset_impl(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts) {
  const auto layout = lookup_scalar_byte_offset_layout(type_text, type_decls, structured_layouts);
  if (layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  return ScalarLayoutByteOffsetFacts{
      .object_size_bytes = layout.size_bytes,
      .target_byte_offset = target_offset,
      .remaining_object_bytes = layout.size_bytes - target_offset,
      .leaf = resolve_scalar_layout_leaf_facts_at_byte_offset(
          type_text, target_offset, type_decls, structured_layouts, 0),
  };
}

std::optional<std::vector<std::string>> collect_local_scalar_array_slots_impl(
    std::string_view type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const std::optional<BirFunctionLowerer::AggregateArrayExtent>& repeated_extent,
    const BirFunctionLowerer::LocalAggregateSlots& aggregate_slots) {
  const auto layout = lookup_scalar_byte_offset_layout(
      c4c::codegen::lir::trim_lir_arg_text(type_text), type_decls, structured_layouts);
  if (layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout =
      lookup_scalar_byte_offset_layout(layout.element_type_text, type_decls, structured_layouts);
  if (element_layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Scalar ||
      element_layout.size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> element_slots;
  std::size_t slot_count = layout.array_count;
  if (repeated_extent.has_value() && repeated_extent->element_stride_bytes == layout.size_bytes) {
    slot_count *= repeated_extent->element_count;
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

}  // namespace

std::optional<ScalarLayoutByteOffsetFacts> resolve_scalar_layout_facts_at_byte_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls) {
  return resolve_scalar_layout_facts_at_byte_offset_impl(
      type_text, target_offset, type_decls, nullptr);
}

std::optional<ScalarLayoutByteOffsetFacts> resolve_scalar_layout_facts_at_byte_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts) {
  return resolve_scalar_layout_facts_at_byte_offset_impl(
      type_text, target_offset, type_decls, &structured_layouts);
}

std::optional<std::vector<std::string>> BirFunctionLowerer::collect_local_scalar_array_slots(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  const auto repeated_extent = find_nested_repeated_aggregate_extent_at_offset(
      aggregate_slots.storage_type_text,
      aggregate_slots.base_byte_offset,
      type_text,
      type_decls);
  return collect_local_scalar_array_slots_impl(
      type_text, type_decls, nullptr, repeated_extent, aggregate_slots);
}

std::optional<std::vector<std::string>> BirFunctionLowerer::collect_local_scalar_array_slots(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts,
    const LocalAggregateSlots& aggregate_slots) {
  const auto repeated_extent = find_nested_repeated_aggregate_extent_at_offset(
      aggregate_slots.storage_type_text,
      aggregate_slots.base_byte_offset,
      type_text,
      type_decls,
      structured_layouts);
  return collect_local_scalar_array_slots_impl(
      type_text, type_decls, &structured_layouts, repeated_extent, aggregate_slots);
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

bool BirFunctionLowerer::lower_local_memory_alloca_inst(
    const c4c::codegen::lir::LirAllocaOp& alloca,
    std::vector<bir::Inst>* lowered_insts) {
  if (alloca.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return false;
  }

  const std::string slot_name = alloca.result.str();
  const auto align_bytes = alloca.align > 0 ? static_cast<std::size_t>(alloca.align) : 0;
  if (!alloca.count.str().empty()) {
    if (!context_.options.preserve_dynamic_alloca) {
      return false;
    }
    const auto element_type = lower_scalar_or_function_pointer_type(alloca.type_str.str());
    const auto lowered_count = lower_value(alloca.count, bir::TypeKind::I64, value_aliases_);
    if (!element_type.has_value() || !lowered_count.has_value()) {
      return false;
    }
    const auto type_text =
        std::string(c4c::codegen::lir::trim_lir_arg_text(alloca.type_str.str()));
    lowered_insts->push_back(bir::CallInst{
        .result = bir::Value::named(bir::TypeKind::Ptr, slot_name),
        .callee = "llvm.dynamic_alloca." + type_text,
        .args = {*lowered_count},
        .arg_types = {bir::TypeKind::I64},
        .return_type = bir::TypeKind::Ptr,
    });
    pointer_value_addresses_[slot_name] = PointerAddress{
        .base_value = bir::Value::named(bir::TypeKind::Ptr, slot_name),
        .value_type = *element_type,
        .byte_offset = 0,
        .storage_type_text = type_text,
        .type_text = type_text,
    };
    return true;
  }

  const auto slot_type = lower_scalar_or_function_pointer_type(alloca.type_str.str());
  if (local_slot_types_.find(slot_name) != local_slot_types_.end() ||
      local_array_slots_.find(slot_name) != local_array_slots_.end()) {
    return false;
  }

  if (slot_type.has_value()) {
    local_slot_types_.emplace(slot_name, *slot_type);
    local_pointer_slots_.emplace(slot_name, slot_name);
    lowered_function_.local_slots.push_back(bir::LocalSlot{
        .name = slot_name,
        .type = *slot_type,
        .align_bytes = align_bytes,
    });
    return true;
  }

  const auto array_type = parse_local_array_type(alloca.type_str.str());
  if (array_type.has_value()) {
    LocalArraySlots array_slots{.element_type = array_type->second};
    array_slots.element_slots.reserve(array_type->first);
    for (std::size_t index = 0; index < array_type->first; ++index) {
      const std::string element_slot = slot_name + "." + std::to_string(index);
      local_slot_types_.emplace(element_slot, array_type->second);
      local_pointer_slots_.emplace(element_slot, element_slot);
      array_slots.element_slots.push_back(element_slot);
      lowered_function_.local_slots.push_back(bir::LocalSlot{
          .name = element_slot,
          .type = array_type->second,
          .align_bytes = align_bytes,
      });
    }
    local_array_slots_.emplace(slot_name, std::move(array_slots));
    return true;
  }

  return declare_local_aggregate_slots(alloca.type_str.str(), slot_name, align_bytes);
}

bool BirFunctionLowerer::lower_memory_store_inst(
    const c4c::codegen::lir::LirStoreOp& store,
    std::vector<bir::Inst>* lowered_insts) {
  if (store.ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
      store.ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
    return false;
  }

  const auto clear_local_scalar_slot_values = [&]() { local_scalar_slot_values_.clear(); };
  const auto erase_local_scalar_slot_value = [&](std::string_view ptr_name) {
    const auto ptr_it = local_pointer_slots_.find(std::string(ptr_name));
    if (ptr_it != local_pointer_slots_.end()) {
      local_scalar_slot_values_.erase(ptr_it->second);
    }
  };

  const auto value_type = lower_scalar_or_function_pointer_type(store.type_str.str());
  if (!value_type.has_value()) {
    const auto aggregate_layout =
        lower_byval_aggregate_layout(store.type_str.str(), type_decls_, &structured_layouts_);
    if (!aggregate_layout.has_value() ||
        store.ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        store.val.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto target_aggregate_it = local_aggregate_slots_.find(store.ptr.str());
    if (target_aggregate_it == local_aggregate_slots_.end()) {
      return false;
    }

    if (const auto source_param_it = aggregate_params_.find(store.val.str());
        source_param_it != aggregate_params_.end()) {
      clear_local_scalar_slot_values();
      const auto leaf_slots = collect_sorted_leaf_slots(target_aggregate_it->second);
      for (const auto& [byte_offset, slot_name] : leaf_slots) {
        const auto slot_type_it = local_slot_types_.find(slot_name);
        if (slot_type_it == local_slot_types_.end()) {
          return false;
        }
        const auto slot_size = type_size_bytes(slot_type_it->second);
        if (slot_size == 0) {
          return false;
        }

        const std::string temp_name =
            store.ptr.str() + ".byval.copy." + std::to_string(byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(slot_type_it->second, temp_name),
            .slot_name = slot_name,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = bir::Value::named(bir::TypeKind::Ptr, store.val.str()),
                    .byte_offset = static_cast<std::int64_t>(byte_offset),
                    .size_bytes = slot_size,
                    .align_bytes = std::max(slot_size, source_param_it->second.layout.align_bytes),
                },
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = slot_name,
            .value = bir::Value::named(slot_type_it->second, temp_name),
        });
      }
      return true;
    }

    const auto source_alias_it = aggregate_value_aliases_.find(store.val.str());
    if (source_alias_it == aggregate_value_aliases_.end()) {
      return false;
    }
    clear_local_scalar_slot_values();
    const auto source_aggregate_it = local_aggregate_slots_.find(source_alias_it->second);
    if (source_aggregate_it == local_aggregate_slots_.end()) {
      return false;
    }
    return append_local_aggregate_copy_from_slots(source_aggregate_it->second,
                                                 target_aggregate_it->second,
                                                 store.ptr.str() + ".aggregate.copy",
                                                 lowered_insts);
  }

  std::optional<bir::Value> value;
  if (*value_type == bir::TypeKind::Ptr &&
      store.val.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = store.val.str().substr(1);
    if (is_known_function_symbol(global_name, function_symbols_)) {
      value = bir::Value::named(bir::TypeKind::Ptr, "@" + global_name);
    }
  }
  if (!value.has_value()) {
    value = lower_value(store.val, *value_type, value_aliases_);
  }
  if (!value.has_value()) {
    return false;
  }

  if (const auto pointer_store = try_lower_pointer_provenance_store(store.ptr.str(),
                                                                    *value_type,
                                                                    *value,
                                                                    type_decls_,
                                                                    local_slot_types_,
                                                                    local_slot_pointer_values_,
                                                                    pointer_value_addresses_,
                                                                    lowered_insts);
      pointer_store.has_value()) {
    clear_local_scalar_slot_values();
    return *pointer_store;
  }

  if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays_.find(store.ptr.str());
      dynamic_ptr_it != dynamic_pointer_value_arrays_.end()) {
    clear_local_scalar_slot_values();
    return append_dynamic_pointer_value_array_store(
        store.ptr.str(), *value_type, *value, dynamic_ptr_it->second, lowered_insts);
  }

  bool handled_dynamic_local_aggregate_store = false;
  if (!try_lower_dynamic_local_aggregate_store(store.ptr.str(),
                                               *value_type,
                                               *value,
                                               dynamic_local_aggregate_arrays_,
                                               type_decls_,
                                               structured_layouts_,
                                               local_slot_types_,
                                               lowered_insts,
                                               &handled_dynamic_local_aggregate_store)) {
    return false;
  }
  if (handled_dynamic_local_aggregate_store) {
    clear_local_scalar_slot_values();
    return true;
  }

  if (const auto global_store = try_lower_global_provenance_store(
          store,
          *value_type,
          *value,
          type_decls_,
          global_types_,
          function_symbols_,
          global_pointer_slots_,
          global_object_pointer_slots_,
          pointer_value_addresses_,
          &global_address_slots_,
          &addressed_global_pointer_slots_,
          &global_pointer_value_slots_,
          &addressed_global_pointer_value_slots_,
          lowered_insts);
      global_store.has_value()) {
    clear_local_scalar_slot_values();
    return *global_store;
  }

  const auto local_slot_store = try_lower_local_slot_store(store.ptr.str(),
                                                          store.val,
                                                          *value_type,
                                                          *value,
                                                          value_aliases_,
                                                          type_decls_,
                                                          global_types_,
                                                          function_symbols_,
                                                          local_pointer_slots_,
                                                          local_slot_types_,
                                                          local_aggregate_field_slots_,
                                                          local_array_slots_,
                                                          local_aggregate_slots_,
                                                          local_pointer_array_bases_,
                                                          local_slot_pointer_values_,
                                                          pointer_value_addresses_,
                                                          global_pointer_slots_,
                                                          global_address_ints_,
                                                          &local_pointer_value_aliases_,
                                                          &local_indirect_pointer_slots_,
                                                          &local_pointer_slot_addresses_,
                                                          &local_slot_address_slots_,
                                                          &local_address_slots_,
                                                          lowered_insts);
  if (local_slot_store == LocalSlotStoreResult::NotHandled) {
    return false;
  }
  if (local_slot_store == LocalSlotStoreResult::Lowered) {
    const auto ptr_it = local_pointer_slots_.find(store.ptr.str());
    if (ptr_it != local_pointer_slots_.end() && *value_type != bir::TypeKind::Ptr &&
        value->kind == bir::Value::Kind::Immediate) {
      local_scalar_slot_values_[ptr_it->second] = *value;
    } else {
      erase_local_scalar_slot_value(store.ptr.str());
    }
    return true;
  }
  erase_local_scalar_slot_value(store.ptr.str());
  return false;
}

bool BirFunctionLowerer::lower_memory_load_inst(
    const c4c::codegen::lir::LirLoadOp& load,
    std::vector<bir::Inst>* lowered_insts) {
  if (load.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      (load.ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
       load.ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
    return false;
  }

  const auto value_type = lower_scalar_or_function_pointer_type(load.type_str.str());
  if (!value_type.has_value()) {
    const auto aggregate_layout =
        lower_byval_aggregate_layout(load.type_str.str(), type_decls_, &structured_layouts_);
    if (!aggregate_layout.has_value()) {
      return false;
    }
    if (load.ptr.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      if (local_aggregate_slots_.find(load.ptr.str()) == local_aggregate_slots_.end()) {
        return false;
      }
      aggregate_value_aliases_[load.result.str()] = load.ptr.str();
      return true;
    }
    if (load.ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
      return false;
    }

    const std::string global_name = load.ptr.str().substr(1);
    const auto global_it = global_types_.find(global_name);
    if (global_it == global_types_.end() || !global_it->second.supports_linear_addressing ||
        global_it->second.storage_size_bytes < aggregate_layout->size_bytes) {
      return false;
    }

    if (!declare_local_aggregate_slots(
            load.type_str.str(), load.result.str(), aggregate_layout->align_bytes)) {
      return false;
    }
    const auto aggregate_it = local_aggregate_slots_.find(load.result.str());
    if (aggregate_it == local_aggregate_slots_.end()) {
      return false;
    }

    const auto leaf_slots = collect_sorted_leaf_slots(aggregate_it->second);
    for (const auto& [byte_offset, slot_name] : leaf_slots) {
      const auto slot_type_it = local_slot_types_.find(slot_name);
      if (slot_type_it == local_slot_types_.end()) {
        return false;
      }
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return false;
      }

      const std::string temp_name =
          load.result.str() + ".global.aggregate.load." + std::to_string(byte_offset);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(slot_type_it->second, temp_name),
          .slot_name = slot_name,
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                  .base_name = global_name,
                  .byte_offset = static_cast<std::int64_t>(byte_offset),
                  .size_bytes = slot_size,
                  .align_bytes = std::max(slot_size, aggregate_layout->align_bytes),
              },
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = slot_name,
          .value = bir::Value::named(slot_type_it->second, temp_name),
      });
    }
    aggregate_value_aliases_[load.result.str()] = load.result.str();
    return true;
  }

  loaded_local_scalar_immediates_.erase(load.result.str());
  if (load.ptr.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
      *value_type != bir::TypeKind::Ptr) {
    const auto ptr_it = local_pointer_slots_.find(load.ptr.str());
    if (ptr_it != local_pointer_slots_.end()) {
      const auto slot_value_it = local_scalar_slot_values_.find(ptr_it->second);
      if (slot_value_it != local_scalar_slot_values_.end() &&
          slot_value_it->second.type == *value_type) {
        loaded_local_scalar_immediates_[load.result.str()] = slot_value_it->second;
      }
    }
  }

  if (const auto global_load = try_lower_global_provenance_load(
          load,
          *value_type,
          global_types_,
          type_decls_,
          global_address_slots_,
          addressed_global_pointer_slots_,
          global_pointer_value_slots_,
          addressed_global_pointer_value_slots_,
          &global_pointer_slots_,
          &global_object_pointer_slots_,
          &pointer_value_addresses_,
          lowered_insts);
      global_load.has_value()) {
    return *global_load;
  }

  if (const auto pointer_load = try_lower_pointer_provenance_load(load.result.str(),
                                                                  load.ptr.str(),
                                                                  *value_type,
                                                                  type_decls_,
                                                                  local_slot_types_,
                                                                  local_indirect_pointer_slots_,
                                                                  local_address_slots_,
                                                                  local_slot_address_slots_,
                                                                  global_types_,
                                                                  function_symbols_,
                                                                  &value_aliases_,
                                                                  &local_slot_pointer_values_,
                                                                  &global_pointer_slots_,
                                                                  pointer_value_addresses_,
                                                                  lowered_insts);
      pointer_load.has_value()) {
    return *pointer_load;
  }

  if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays_.find(load.ptr.str());
      dynamic_ptr_it != dynamic_pointer_value_arrays_.end()) {
    const auto selected_value = load_dynamic_pointer_value_array_value(
        load.result.str(), *value_type, dynamic_ptr_it->second, lowered_insts);
    if (!selected_value.has_value()) {
      return false;
    }
    if (selected_value->kind == bir::Value::Kind::Named &&
        selected_value->name == load.result.str()) {
      return true;
    }
    value_aliases_[load.result.str()] = *selected_value;
    return true;
  }

  if (const auto global_scalar_it = dynamic_global_scalar_arrays_.find(load.ptr.str());
      global_scalar_it != dynamic_global_scalar_arrays_.end()) {
    const auto selected_value = load_dynamic_global_scalar_array_value(
        load.result.str(), *value_type, global_scalar_it->second, lowered_insts);
    if (!selected_value.has_value()) {
      return false;
    }
    if (selected_value->kind == bir::Value::Kind::Named &&
        selected_value->name == load.result.str()) {
      return true;
    }
    value_aliases_[load.result.str()] = *selected_value;
    return true;
  }

  bool handled_dynamic_local_aggregate_load = false;
  if (!try_lower_dynamic_local_aggregate_load(load.result.str(),
                                              load.ptr.str(),
                                              *value_type,
                                              dynamic_local_aggregate_arrays_,
                                              type_decls_,
                                              structured_layouts_,
                                              local_slot_types_,
                                              &value_aliases_,
                                              lowered_insts,
                                              &handled_dynamic_local_aggregate_load)) {
    return false;
  }
  if (handled_dynamic_local_aggregate_load) {
    return true;
  }

  const auto local_slot_load = try_lower_local_slot_load(load.result.str(),
                                                        load.ptr.str(),
                                                        *value_type,
                                                        local_pointer_slots_,
                                                        local_slot_types_,
                                                        local_aggregate_field_slots_,
                                                        local_array_slots_,
                                                        local_pointer_value_aliases_,
                                                        type_decls_,
                                                        local_indirect_pointer_slots_,
                                                        local_address_slots_,
                                                        local_slot_address_slots_,
                                                        local_pointer_slot_addresses_,
                                                        global_types_,
                                                        function_symbols_,
                                                        &value_aliases_,
                                                        &local_slot_pointer_values_,
                                                        &local_aggregate_slots_,
                                                        &local_pointer_array_bases_,
                                                        &global_pointer_slots_,
                                                        &pointer_value_addresses_,
                                                        &global_address_ints_,
                                                        lowered_insts);
  if (local_slot_load == LocalSlotLoadResult::NotHandled) {
    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto dynamic_pointer_array_load = try_lower_dynamic_pointer_array_load(
              load.result.str(),
              load.ptr.str(),
              dynamic_local_pointer_arrays_,
              dynamic_global_pointer_arrays_,
              local_pointer_value_aliases_,
              global_types_,
              &value_aliases_,
              lowered_insts);
          dynamic_pointer_array_load.has_value()) {
        return *dynamic_pointer_array_load;
      }
    }
    return false;
  }
  return local_slot_load == LocalSlotLoadResult::Lowered;
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
                    local_aggregate_it->second.type_text,
                    type_decls,
                    structured_layouts_,
                    local_aggregate_it->second);
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
            lookup_backend_aggregate_type_layout(local_aggregate_it->second.type_text,
                                                 type_decls,
                                                 structured_layouts_);
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
                local_aggregate_it->second.type_text,
                type_decls,
                structured_layouts_,
                local_aggregate_it->second);
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
                                         structured_layouts_,
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
    const BackendStructuredLayoutTable& structured_layouts,
    LocalSlotPointerValues* local_slot_pointer_values,
    LocalAggregateSlotMap* local_aggregate_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases) {
  record_loaded_local_pointer_slot_state(result_name,
                                         slot_name,
                                         local_slot_address_slots,
                                         type_decls,
                                         &structured_layouts,
                                         local_slot_pointer_values,
                                         local_aggregate_slots,
                                         local_pointer_array_bases);
}

void BirFunctionLowerer::record_loaded_local_pointer_slot_state(
    std::string_view result_name,
    std::string_view slot_name,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const TypeDeclMap& type_decls,
    LocalSlotPointerValues* local_slot_pointer_values,
    LocalAggregateSlotMap* local_aggregate_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases) {
  record_loaded_local_pointer_slot_state(result_name,
                                         slot_name,
                                         local_slot_address_slots,
                                         type_decls,
                                         nullptr,
                                         local_slot_pointer_values,
                                         local_aggregate_slots,
                                         local_pointer_array_bases);
}

void BirFunctionLowerer::record_loaded_local_pointer_slot_state(
    std::string_view result_name,
    std::string_view slot_name,
    const LocalSlotAddressSlots& local_slot_address_slots,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    LocalSlotPointerValues* local_slot_pointer_values,
    LocalAggregateSlotMap* local_aggregate_slots,
    LocalPointerArrayBaseMap* local_pointer_array_bases) {
  const auto local_slot_it = local_slot_address_slots.find(std::string(slot_name));
  if (local_slot_it == local_slot_address_slots.end()) {
    return;
  }

  const auto result = std::string(result_name);
  (*local_slot_pointer_values)[result] = local_slot_it->second;
  const auto loaded_layout = lookup_scalar_byte_offset_layout(
      local_slot_it->second.type_text, type_decls, structured_layouts);
  if (loaded_layout.kind == AggregateTypeLayout::Kind::Array &&
      !local_slot_it->second.array_element_slots.empty() &&
      local_slot_it->second.byte_offset >= 0) {
    const auto element_layout = lookup_scalar_byte_offset_layout(
        loaded_layout.element_type_text, type_decls, structured_layouts);
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
    const BackendStructuredLayoutTable& structured_layouts,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  return load_dynamic_local_aggregate_array_value(result_name,
                                                  value_type,
                                                  access,
                                                  type_decls,
                                                  &structured_layouts,
                                                  local_slot_types,
                                                  lowered_insts);
}

std::optional<bir::Value> BirFunctionLowerer::load_dynamic_local_aggregate_array_value(
    std::string_view result_name,
    bir::TypeKind value_type,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_count == 0) {
    return std::nullopt;
  }

  const auto element_layout =
      lookup_scalar_byte_offset_layout(access.element_type_text, type_decls, structured_layouts);
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

std::optional<bir::Value> BirFunctionLowerer::load_dynamic_local_aggregate_array_value(
    std::string_view result_name,
    bir::TypeKind value_type,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  return load_dynamic_local_aggregate_array_value(result_name,
                                                  value_type,
                                                  access,
                                                  type_decls,
                                                  nullptr,
                                                  local_slot_types,
                                                  lowered_insts);
}

bool BirFunctionLowerer::append_dynamic_local_aggregate_store(
    std::string_view scratch_prefix,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  return append_dynamic_local_aggregate_store(scratch_prefix,
                                              value_type,
                                              value,
                                              access,
                                              type_decls,
                                              &structured_layouts,
                                              local_slot_types,
                                              lowered_insts);
}

bool BirFunctionLowerer::append_dynamic_local_aggregate_store(
    std::string_view scratch_prefix,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable* structured_layouts,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_count == 0) {
    return false;
  }

  const auto element_layout =
      lookup_scalar_byte_offset_layout(access.element_type_text, type_decls, structured_layouts);
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

bool BirFunctionLowerer::append_dynamic_local_aggregate_store(
    std::string_view scratch_prefix,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicLocalAggregateArrayAccess& access,
    const TypeDeclMap& type_decls,
    const LocalSlotTypes& local_slot_types,
    std::vector<bir::Inst>* lowered_insts) {
  return append_dynamic_local_aggregate_store(scratch_prefix,
                                              value_type,
                                              value,
                                              access,
                                              type_decls,
                                              nullptr,
                                              local_slot_types,
                                              lowered_insts);
}

bool BirFunctionLowerer::try_lower_dynamic_local_aggregate_store(
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts,
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
                                              structured_layouts,
                                              local_slot_types,
                                              lowered_insts);
}

bool BirFunctionLowerer::try_lower_dynamic_local_aggregate_load(
    std::string_view result_name,
    std::string_view ptr_name,
    bir::TypeKind value_type,
    const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts,
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
                                                                       structured_layouts,
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
