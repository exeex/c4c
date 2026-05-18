#include "../module.hpp"
#include "stack_layout.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace c4c::backend::prepare {

namespace {

using FrameSlotMap = std::unordered_map<SlotNameId, const PreparedFrameSlot*>;

struct ResolvedFrameSlot {
  const PreparedFrameSlot* frame_slot = nullptr;
  std::int64_t byte_offset_adjust = 0;
};

[[nodiscard]] bir::AddressSpace prepared_memory_address_space(
    const std::optional<bir::MemoryAddress>& address) {
  return address.has_value() ? address->address_space : bir::AddressSpace::Default;
}

[[nodiscard]] bool prepared_memory_is_volatile(
    const std::optional<bir::MemoryAddress>& address) {
  return address.has_value() && address->is_volatile;
}

[[nodiscard]] const bir::Global* find_global_by_link_name_id(const bir::Module& module,
                                                             LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return nullptr;
  }
  for (const auto& global : module.globals) {
    if (global.link_name_id == link_name_id) {
      return &global;
    }
  }
  return nullptr;
}

[[nodiscard]] bool module_has_function_link_name_id(const bir::Module& module,
                                                    LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return false;
  }
  for (const auto& function : module.functions) {
    if (function.link_name_id == link_name_id) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] const bir::Function* find_function_by_link_name_id(const bir::Module& module,
                                                                 LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return nullptr;
  }
  for (const auto& function : module.functions) {
    if (function.link_name_id == link_name_id) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<LinkNameId> resolve_prepared_link_name_id(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return std::nullopt;
  }
  const std::string_view spelling = bir_names.link_names.spelling(link_name_id);
  if (spelling.empty()) {
    return std::nullopt;
  }
  return names.link_names.intern(spelling);
}

[[nodiscard]] std::optional<TextId> resolve_prepared_text_id(
    PreparedNameTables& names,
    const bir::Module& module,
    std::string_view text_name) {
  if (text_name.empty()) {
    return std::nullopt;
  }
  for (const auto& string_constant : module.string_constants) {
    if (string_constant.name == text_name) {
      const std::string_view structured_name =
          module.names.texts.lookup(string_constant.name_id);
      return names.texts.intern(structured_name.empty() ? text_name : structured_name);
    }
  }
  return names.texts.intern(text_name);
}

[[nodiscard]] BlockLabelId intern_preferred_block_label(PreparedNameTables& names,
                                                        const bir::NameTables& bir_names,
                                                        BlockLabelId block_label_id,
                                                        std::string_view raw_label) {
  if (block_label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block_label_id);
    if (!structured_label.empty()) {
      return names.block_labels.intern(structured_label);
    }
  }
  return names.block_labels.intern(raw_label);
}

[[nodiscard]] std::optional<std::pair<std::string_view, std::size_t>> parse_slot_slice_name(
    std::string_view slot_name) {
  const auto dot = slot_name.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= slot_name.size()) {
    return std::nullopt;
  }

  std::size_t slice_offset = 0;
  const auto suffix = slot_name.substr(dot + 1);
  const auto* begin = suffix.data();
  const auto* end = begin + suffix.size();
  const auto [ptr, ec] = std::from_chars(begin, end, slice_offset);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return std::pair<std::string_view, std::size_t>{slot_name.substr(0, dot), slice_offset};
}

[[nodiscard]] FrameSlotMap build_frame_slot_map(
    const std::vector<PreparedStackObject>& function_objects,
    const std::vector<PreparedFrameSlot>& function_slots) {
  std::unordered_map<PreparedObjectId, SlotNameId> slot_names_by_object_id;
  slot_names_by_object_id.reserve(function_objects.size());
  for (const auto& object : function_objects) {
    if (!object.slot_name.has_value()) {
      continue;
    }
    slot_names_by_object_id.emplace(object.object_id, *object.slot_name);
  }

  FrameSlotMap frame_slots_by_name;
  frame_slots_by_name.reserve(function_slots.size());
  for (const auto& slot : function_slots) {
    const auto name_it = slot_names_by_object_id.find(slot.object_id);
    if (name_it == slot_names_by_object_id.end()) {
      continue;
    }
    frame_slots_by_name.emplace(name_it->second, &slot);
  }
  return frame_slots_by_name;
}

[[nodiscard]] ResolvedFrameSlot find_direct_frame_slot(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    std::string_view slot_name,
    SlotNameId slot_id,
    const std::optional<bir::MemoryAddress>& address,
    std::int64_t access_byte_offset,
    std::size_t access_size_bytes,
    const FrameSlotMap& frame_slots_by_name) {
  std::string_view requested_slot_name = slot_name;
  SlotNameId requested_slot_id = slot_id;
  if (address.has_value()) {
    if (address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot ||
        address->base_name.empty()) {
      return {};
    }
    requested_slot_name = address->base_name;
    requested_slot_id = address->base_slot_id;
  }

  ResolvedFrameSlot resolved;
  const SlotNameId prepared_slot_id =
      intern_prepared_slot_name(names, bir_names, requested_slot_id, requested_slot_name);
  if (const auto slot_it = frame_slots_by_name.find(prepared_slot_id);
      slot_it != frame_slots_by_name.end()) {
    resolved.frame_slot = slot_it->second;
  }

  const auto requested_slice = parse_slot_slice_name(requested_slot_name);
  if (!requested_slice.has_value()) {
    return resolved;
  }

  const auto requested_begin =
      static_cast<std::int64_t>(requested_slice->second) + access_byte_offset;
  const auto requested_end = requested_begin + static_cast<std::int64_t>(access_size_bytes);
  if (requested_begin < 0 || requested_end < requested_begin) {
    return resolved;
  }

  std::optional<std::size_t> best_covering_offset;
  for (const auto& [candidate_name_id, candidate_slot] : frame_slots_by_name) {
    const auto candidate_name = prepared_slot_name(names, candidate_name_id);
    const auto candidate_slice = parse_slot_slice_name(candidate_name);
    if (!candidate_slice.has_value() || candidate_slice->first != requested_slice->first) {
      continue;
    }

    const auto candidate_begin = static_cast<std::int64_t>(candidate_slice->second);
    const auto candidate_end =
        candidate_begin + static_cast<std::int64_t>(candidate_slot->size_bytes);
    if (requested_begin < candidate_begin || requested_end > candidate_end) {
      continue;
    }

    if (!best_covering_offset.has_value() || candidate_slice->second < *best_covering_offset) {
      best_covering_offset = candidate_slice->second;
      resolved.frame_slot = candidate_slot;
      resolved.byte_offset_adjust =
          static_cast<std::int64_t>(requested_slice->second) - candidate_begin;
    }
  }

  return resolved;
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_frame_slot_access(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::LoadLocalInst& inst,
    const FrameSlotMap& frame_slots_by_name) {
  if (inst.address.has_value() &&
      inst.address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.result.type, inst.result.type == bir::TypeKind::Void
                                                         ? 0
                                                         : stack_layout::fallback_type_size(
                                                               inst.result.type));
  const auto access_byte_offset =
      static_cast<std::int64_t>(inst.byte_offset) +
      (inst.address.has_value() ? inst.address->byte_offset : 0);
  const auto resolved_frame_slot =
      find_direct_frame_slot(
          names,
          bir_names,
          inst.slot_name,
          inst.slot_id,
          inst.address,
          access_byte_offset,
          size_bytes,
          frame_slots_by_name);
  if (resolved_frame_slot.frame_slot == nullptr) {
    return std::nullopt;
  }
  const auto byte_offset = access_byte_offset + resolved_frame_slot.byte_offset_adjust;
  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address =
          PreparedAddress{
              .base_kind = PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = resolved_frame_slot.frame_slot->slot_id,
              .byte_offset = byte_offset,
              .size_bytes = size_bytes,
              .align_bytes = stack_layout::normalize_alignment(
                  inst.result.type, inst.align_bytes, size_bytes),
              .can_use_base_plus_offset = true,
          },
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_frame_slot_access(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::StoreLocalInst& inst,
    const FrameSlotMap& frame_slots_by_name) {
  if (inst.address.has_value() &&
      inst.address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.value.type, inst.value.type == bir::TypeKind::Void
                                                        ? 0
                                                        : stack_layout::fallback_type_size(
                                                              inst.value.type));
  const auto access_byte_offset =
      static_cast<std::int64_t>(inst.byte_offset) +
      (inst.address.has_value() ? inst.address->byte_offset : 0);
  const auto resolved_frame_slot =
      find_direct_frame_slot(
          names,
          bir_names,
          inst.slot_name,
          inst.slot_id,
          inst.address,
          access_byte_offset,
          size_bytes,
          frame_slots_by_name);
  if (resolved_frame_slot.frame_slot == nullptr) {
    return std::nullopt;
  }
  const auto byte_offset = access_byte_offset + resolved_frame_slot.byte_offset_adjust;
  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address =
          PreparedAddress{
              .base_kind = PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = resolved_frame_slot.frame_slot->slot_id,
              .byte_offset = byte_offset,
              .size_bytes = size_bytes,
              .align_bytes = stack_layout::normalize_alignment(
                  inst.value.type, inst.align_bytes, size_bytes),
              .can_use_base_plus_offset = true,
          },
  };
}

[[nodiscard]] std::optional<PreparedAddress> build_direct_symbol_backed_address(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const std::optional<bir::MemoryAddress>& address,
    std::string_view fallback_symbol_name,
    LinkNameId fallback_symbol_link_name_id,
    std::int64_t fallback_byte_offset,
    std::size_t size_bytes,
    std::size_t align_bytes) {
  const auto resolve_symbol_name =
      [&](std::string_view raw_symbol_name,
          LinkNameId symbol_link_name_id) -> std::optional<LinkNameId> {
    if (symbol_link_name_id != kInvalidLinkName) {
      const std::string_view semantic_symbol_name =
          bir_names.link_names.spelling(symbol_link_name_id);
      if (semantic_symbol_name.empty()) {
        return std::nullopt;
      }
      if (!raw_symbol_name.empty() && raw_symbol_name != semantic_symbol_name) {
        return std::nullopt;
      }
      return names.link_names.intern(semantic_symbol_name);
    }
    if (raw_symbol_name.empty()) {
      return std::nullopt;
    }
    return names.link_names.intern(raw_symbol_name);
  };

  if (!address.has_value()) {
    const auto symbol_name =
        resolve_symbol_name(fallback_symbol_name, fallback_symbol_link_name_id);
    if (!symbol_name.has_value()) {
      return std::nullopt;
    }
    return PreparedAddress{
        .base_kind = PreparedAddressBaseKind::GlobalSymbol,
        .symbol_name = *symbol_name,
        .byte_offset = fallback_byte_offset,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .can_use_base_plus_offset = true,
    };
  }

  PreparedAddressBaseKind base_kind = PreparedAddressBaseKind::None;
  switch (address->base_kind) {
    case bir::MemoryAddress::BaseKind::GlobalSymbol:
      base_kind = PreparedAddressBaseKind::GlobalSymbol;
      break;
    case bir::MemoryAddress::BaseKind::StringConstant:
      base_kind = PreparedAddressBaseKind::StringConstant;
      break;
    default:
      return std::nullopt;
  }

  const std::string_view symbol_name =
      address->base_name.empty() ? fallback_symbol_name : std::string_view(address->base_name);
  const LinkNameId symbol_link_name_id =
      address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol
          ? (address->base_link_name_id != kInvalidLinkName
                 ? address->base_link_name_id
                 : (address->base_name.empty() ? fallback_symbol_link_name_id : kInvalidLinkName))
          : kInvalidLinkName;
  const auto prepared_symbol_name = resolve_symbol_name(symbol_name, symbol_link_name_id);
  if (!prepared_symbol_name.has_value()) {
    return std::nullopt;
  }

  return PreparedAddress{
      .base_kind = base_kind,
      .symbol_name = *prepared_symbol_name,
      .byte_offset = address->byte_offset + fallback_byte_offset,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes == 0 ? address->align_bytes : align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::LoadLocalInst& inst) {
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.result.type, inst.result.type == bir::TypeKind::Void
                                                         ? 0
                                                         : stack_layout::fallback_type_size(
                                                               inst.result.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.result.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes : inst.align_bytes,
      size_bytes);
  auto address = build_direct_symbol_backed_address(
      names,
      bir_names,
      inst.address,
      {},
      kInvalidLinkName,
      static_cast<std::int64_t>(inst.byte_offset),
      size_bytes,
      align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::StoreLocalInst& inst) {
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.value.type, inst.value.type == bir::TypeKind::Void
                                                        ? 0
                                                        : stack_layout::fallback_type_size(
                                                              inst.value.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.value.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes : inst.align_bytes,
      size_bytes);
  auto address = build_direct_symbol_backed_address(
      names,
      bir_names,
      inst.address,
      {},
      kInvalidLinkName,
      static_cast<std::int64_t>(inst.byte_offset),
      size_bytes,
      align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::LoadGlobalInst& inst) {
  if (inst.address.has_value() &&
      inst.address->base_kind == bir::MemoryAddress::BaseKind::PointerValue) {
    return std::nullopt;
  }
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.result.type, inst.result.type == bir::TypeKind::Void
                                                         ? 0
                                                         : stack_layout::fallback_type_size(
                                                               inst.result.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.result.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes : inst.align_bytes,
      size_bytes);
  auto address = build_direct_symbol_backed_address(
      names,
      bir_names,
      inst.address,
      inst.global_name,
      inst.global_name_id,
      static_cast<std::int64_t>(inst.byte_offset),
      size_bytes,
      align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::StoreGlobalInst& inst) {
  if (inst.address.has_value() &&
      inst.address->base_kind == bir::MemoryAddress::BaseKind::PointerValue) {
    return std::nullopt;
  }
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.value.type, inst.value.type == bir::TypeKind::Void
                                                        ? 0
                                                        : stack_layout::fallback_type_size(
                                                              inst.value.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.value.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes : inst.align_bytes,
      size_bytes);
  auto address = build_direct_symbol_backed_address(
      names,
      bir_names,
      inst.address,
      inst.global_name,
      inst.global_name_id,
      static_cast<std::int64_t>(inst.byte_offset),
      size_bytes,
      align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedAddress> build_pointer_indirect_address(
    PreparedNameTables& names,
    const std::optional<bir::MemoryAddress>& address,
    std::size_t size_bytes,
    std::size_t align_bytes) {
  if (!address.has_value() || address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      address->base_value.kind != bir::Value::Kind::Named || address->base_value.name.empty()) {
    return std::nullopt;
  }

  return PreparedAddress{
      .base_kind = PreparedAddressBaseKind::PointerValue,
      .pointer_value_name = prepared_named_value_id(names, address->base_value),
      .byte_offset = address->byte_offset,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes == 0 ? address->align_bytes : align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    PreparedNameTables& names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::LoadLocalInst& inst) {
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.result.type, inst.result.type == bir::TypeKind::Void
                                                         ? 0
                                                         : stack_layout::fallback_type_size(
                                                               inst.result.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.result.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes
                                                        : inst.align_bytes,
      size_bytes);
  auto address = build_pointer_indirect_address(names, inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    PreparedNameTables& names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::StoreLocalInst& inst) {
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.value.type, inst.value.type == bir::TypeKind::Void
                                                        ? 0
                                                        : stack_layout::fallback_type_size(
                                                              inst.value.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.value.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes
                                                        : inst.align_bytes,
      size_bytes);
  auto address = build_pointer_indirect_address(names, inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    PreparedNameTables& names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::LoadGlobalInst& inst) {
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.result.type, inst.result.type == bir::TypeKind::Void
                                                         ? 0
                                                         : stack_layout::fallback_type_size(
                                                               inst.result.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.result.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes
                                                        : inst.align_bytes,
      size_bytes);
  auto address = build_pointer_indirect_address(names, inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    PreparedNameTables& names,
    FunctionNameId function_name_id,
    BlockLabelId block_label_id,
    std::size_t inst_index,
    const bir::StoreGlobalInst& inst) {
  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.value.type, inst.value.type == bir::TypeKind::Void
                                                        ? 0
                                                        : stack_layout::fallback_type_size(
                                                              inst.value.type));
  const std::size_t align_bytes = stack_layout::normalize_alignment(
      inst.value.type,
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes
                                                        : inst.align_bytes,
      size_bytes);
  auto address = build_pointer_indirect_address(names, inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
      .address_space = prepared_memory_address_space(inst.address),
      .is_volatile = prepared_memory_is_volatile(inst.address),
      .address = std::move(*address),
  };
}

void append_direct_frame_slot_accesses(PreparedNameTables& names,
                                       PreparedAddressingFunction& function_addressing,
                                       FunctionNameId function_name_id,
                                       const bir::NameTables& bir_names,
                                       const bir::Function& function,
                                       const FrameSlotMap& frame_slots_by_name) {
  for (const auto& block : function.blocks) {
    const BlockLabelId block_label_id =
        intern_preferred_block_label(names, bir_names, block.label_id, block.label);
    for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
      const auto& inst = block.insts[inst_index];
      if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
        if (auto access =
                build_pointer_indirect_access(names, function_name_id, block_label_id, inst_index, *load_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_symbol_backed_access(
                names, bir_names, function_name_id, block_label_id, inst_index, *load_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_frame_slot_access(names,
                                                         bir_names,
                                                         function_name_id,
                                                         block_label_id,
                                                         inst_index,
                                                         *load_local,
                                                         frame_slots_by_name);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
        continue;
      }
      if (const auto* store_local = std::get_if<bir::StoreLocalInst>(&inst)) {
        if (auto access =
                build_pointer_indirect_access(
                    names, function_name_id, block_label_id, inst_index, *store_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_symbol_backed_access(
                names, bir_names, function_name_id, block_label_id, inst_index, *store_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_frame_slot_access(names,
                                                         bir_names,
                                                         function_name_id,
                                                         block_label_id,
                                                         inst_index,
                                                         *store_local,
                                                         frame_slots_by_name);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
        continue;
      }
      if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
        if (auto access =
                build_pointer_indirect_access(
                    names, function_name_id, block_label_id, inst_index, *load_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_symbol_backed_access(
                names, bir_names, function_name_id, block_label_id, inst_index, *load_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
        continue;
      }
      if (const auto* store_global = std::get_if<bir::StoreGlobalInst>(&inst)) {
        if (auto access = build_pointer_indirect_access(
                names, function_name_id, block_label_id, inst_index, *store_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_symbol_backed_access(
                names, bir_names, function_name_id, block_label_id, inst_index, *store_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
      }
    }
  }
}

void append_missing_address_materialization_fact(std::vector<PrepareNote>& notes,
                                                 std::string message) {
  notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message = std::move(message),
  });
}

void append_direct_global_address_materialization(PreparedNameTables& names,
                                                  PreparedAddressingFunction& function_addressing,
                                                  std::vector<PrepareNote>& notes,
                                                  const bir::Module& module,
                                                  const c4c::TargetProfile& target_profile,
                                                  FunctionNameId function_name_id,
                                                  BlockLabelId block_label_id,
                                                  std::size_t inst_index,
                                                  const bir::Value& result) {
  if (result.type != bir::TypeKind::Ptr || result.kind != bir::Value::Kind::Named) {
    return;
  }
  if (result.pointer_symbol_link_name_id == kInvalidLinkName) {
    if (!result.name.empty() && result.name.front() == '@') {
      append_missing_address_materialization_fact(
          notes,
          "prepared address materialization for '" + result.name +
              "' is missing a structured symbol LinkNameId");
    }
    return;
  }
  const auto prepared_symbol_name =
      resolve_prepared_link_name_id(names, module.names, result.pointer_symbol_link_name_id);
  if (!prepared_symbol_name.has_value()) {
    append_missing_address_materialization_fact(
        notes,
        "prepared address materialization for '" + result.name +
            "' references a missing structured symbol spelling");
    return;
  }
  const bir::Global* global =
      find_global_by_link_name_id(module, result.pointer_symbol_link_name_id);
  const bir::Function* target_function =
      find_function_by_link_name_id(module, result.pointer_symbol_link_name_id);
  if (global == nullptr && target_function == nullptr) {
    append_missing_address_materialization_fact(
        notes,
        "prepared address materialization for '" + result.name +
            "' references an unknown global/function symbol");
    return;
  }
  const auto policy = global != nullptr ? global->address_materialization_policy
                                        : target_function->address_materialization_policy;
  if (policy == bir::GlobalAddressMaterializationPolicy::Unspecified &&
      target_profile.relocation_model != c4c::TargetRelocationModel::Static) {
    append_missing_address_materialization_fact(
        notes,
        "prepared address materialization for '" + result.name +
            "' needs explicit GOT/direct policy for relocation model " +
            c4c::target_relocation_model_name(target_profile.relocation_model));
    return;
  }
  if (policy == bir::GlobalAddressMaterializationPolicy::GotRequired && global != nullptr &&
      global->is_thread_local) {
    append_missing_address_materialization_fact(
        notes,
        "prepared GOT address materialization for '" + result.name +
            "' targets TLS global before TLS/GOT policy is specified");
    return;
  }

  const bool is_tls_global = global != nullptr && global->is_thread_local;
  function_addressing.address_materializations.push_back(PreparedAddressMaterialization{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .kind = policy == bir::GlobalAddressMaterializationPolicy::GotRequired
                  ? PreparedAddressMaterializationKind::GotGlobal
                  : (is_tls_global ? PreparedAddressMaterializationKind::TlsGlobal
                                   : PreparedAddressMaterializationKind::DirectGlobal),
      .result_value_name = prepared_named_value_id(names, result),
      .symbol_name = *prepared_symbol_name,
      .address_materialization_policy =
          policy == bir::GlobalAddressMaterializationPolicy::Unspecified
              ? bir::GlobalAddressMaterializationPolicy::Direct
              : policy,
      .address_space = is_tls_global ? bir::AddressSpace::Tls : bir::AddressSpace::Default,
      .is_thread_local = is_tls_global,
      .has_tls_address_space = is_tls_global,
      .tls_model = is_tls_global
                       ? PreparedTlsMaterializationModel::LocalExecThreadPointerRelative
                       : PreparedTlsMaterializationModel::None,
      .tls_thread_pointer_register =
          is_tls_global ? PreparedTlsThreadPointerRegister::Aarch64TpidrEl0
                        : PreparedTlsThreadPointerRegister::None,
      .tls_high_relocation = is_tls_global ? PreparedTlsRelocationKind::Aarch64TprelHi12
                                           : PreparedTlsRelocationKind::None,
      .tls_low_relocation = is_tls_global ? PreparedTlsRelocationKind::Aarch64TprelLo12Nc
                                          : PreparedTlsRelocationKind::None,
  });
}

void append_string_constant_address_materialization(PreparedNameTables& names,
                                                    PreparedAddressingFunction& function_addressing,
                                                    std::vector<PrepareNote>& notes,
                                                    const bir::Module& module,
                                                    FunctionNameId function_name_id,
                                                    BlockLabelId block_label_id,
                                                    std::size_t inst_index,
                                                    const bir::Value& result,
                                                    const std::optional<bir::MemoryAddress>& address,
                                                    std::int64_t inst_byte_offset) {
  if (result.type != bir::TypeKind::Ptr || result.kind != bir::Value::Kind::Named ||
      !address.has_value() ||
      address->base_kind != bir::MemoryAddress::BaseKind::StringConstant) {
    return;
  }
  const auto prepared_text_name =
      resolve_prepared_text_id(names, module, address->base_name);
  if (!prepared_text_name.has_value()) {
    append_missing_address_materialization_fact(
        notes,
        "prepared string-constant address materialization for '" + result.name +
            "' is missing a string text identity");
    return;
  }
  function_addressing.address_materializations.push_back(PreparedAddressMaterialization{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .kind = PreparedAddressMaterializationKind::StringConstant,
      .result_value_name = prepared_named_value_id(names, result),
      .text_name = *prepared_text_name,
      .byte_offset = address->byte_offset + inst_byte_offset,
      .address_space = address->address_space,
      .has_tls_address_space = address->address_space == bir::AddressSpace::Tls,
  });
}

[[nodiscard]] std::optional<std::string_view> direct_string_constant_name(
    const bir::Module& module,
    const bir::Value& value) {
  if (value.type != bir::TypeKind::Ptr || value.kind != bir::Value::Kind::Named ||
      value.name.empty() || value.name.front() != '@') {
    return std::nullopt;
  }
  std::string_view text_name(value.name.data() + 1, value.name.size() - 1);
  for (const auto& string_constant : module.string_constants) {
    if (string_constant.name == text_name) {
      return text_name;
    }
  }
  return std::nullopt;
}

void append_call_argument_address_materialization(PreparedNameTables& names,
                                                  PreparedAddressingFunction& function_addressing,
                                                  std::vector<PrepareNote>& notes,
                                                  const bir::Module& module,
                                                  const c4c::TargetProfile& target_profile,
                                                  FunctionNameId function_name_id,
                                                  BlockLabelId block_label_id,
                                                  std::size_t inst_index,
                                                  const bir::Value& argument) {
  if (argument.type != bir::TypeKind::Ptr || argument.kind != bir::Value::Kind::Named) {
    return;
  }
  if (argument.pointer_symbol_link_name_id != kInvalidLinkName) {
    append_direct_global_address_materialization(
        names,
        function_addressing,
        notes,
        module,
        target_profile,
        function_name_id,
        block_label_id,
        inst_index,
        argument);
    return;
  }

  const auto text_name = direct_string_constant_name(module, argument);
  if (!text_name.has_value()) {
    return;
  }
  const auto prepared_text_name = resolve_prepared_text_id(names, module, *text_name);
  if (!prepared_text_name.has_value()) {
    append_missing_address_materialization_fact(
        notes,
        "prepared string-constant call-argument address materialization for '" +
            argument.name + "' is missing a string text identity");
    return;
  }
  function_addressing.address_materializations.push_back(PreparedAddressMaterialization{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .kind = PreparedAddressMaterializationKind::StringConstant,
      .result_value_name = prepared_named_value_id(names, argument),
      .text_name = *prepared_text_name,
      .address_space = bir::AddressSpace::Default,
  });
}

[[nodiscard]] std::optional<BlockLabelId> resolve_label_address_target(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Function& function,
    const bir::MemoryAddress& address) {
  if (address.base_kind != bir::MemoryAddress::BaseKind::Label) {
    return std::nullopt;
  }
  if (address.base_label_id != kInvalidBlockLabel) {
    const std::string_view label = bir_names.block_labels.spelling(address.base_label_id);
    if (!label.empty()) {
      for (const auto& block : function.blocks) {
        if (block.label_id == address.base_label_id || block.label == label) {
          return names.block_labels.intern(label);
        }
      }
    }
  }
  if (!address.base_name.empty()) {
    for (const auto& block : function.blocks) {
      if (block.label == address.base_name) {
        return intern_preferred_block_label(names, bir_names, block.label_id, block.label);
      }
    }
  }
  return std::nullopt;
}

void append_label_address_materialization(PreparedNameTables& names,
                                          PreparedAddressingFunction& function_addressing,
                                          std::vector<PrepareNote>& notes,
                                          const bir::Module& module,
                                          const bir::Function& function,
                                          FunctionNameId function_name_id,
                                          BlockLabelId block_label_id,
                                          std::size_t inst_index,
                                          const bir::Value& result,
                                          const std::optional<bir::MemoryAddress>& address,
                                          std::int64_t inst_byte_offset) {
  if (result.type != bir::TypeKind::Ptr || result.kind != bir::Value::Kind::Named ||
      !address.has_value() || address->base_kind != bir::MemoryAddress::BaseKind::Label) {
    return;
  }
  const auto target_label =
      resolve_label_address_target(names, module.names, function, *address);
  if (!target_label.has_value()) {
    append_missing_address_materialization_fact(
        notes,
        "prepared label address materialization for '" + result.name +
            "' is missing a target BlockLabelId");
    return;
  }
  function_addressing.address_materializations.push_back(PreparedAddressMaterialization{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .kind = PreparedAddressMaterializationKind::Label,
      .result_value_name = prepared_named_value_id(names, result),
      .target_label = *target_label,
      .byte_offset = address->byte_offset + inst_byte_offset,
      .address_space = address->address_space,
      .has_tls_address_space = address->address_space == bir::AddressSpace::Tls,
  });
}

void append_address_materializations(PreparedNameTables& names,
                                     PreparedAddressingFunction& function_addressing,
                                     std::vector<PrepareNote>& notes,
                                     const bir::Module& module,
                                     const c4c::TargetProfile& target_profile,
                                     FunctionNameId function_name_id,
                                     const bir::Function& function) {
  for (const auto& block : function.blocks) {
    const BlockLabelId block_label_id =
        intern_preferred_block_label(names, module.names, block.label_id, block.label);
    for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
      const auto& inst = block.insts[inst_index];
      if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        append_direct_global_address_materialization(
            names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, binary->result);
      } else if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
        append_direct_global_address_materialization(
            names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, select->result);
      } else if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
        append_direct_global_address_materialization(
            names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, cast->result);
      } else if (const auto* phi = std::get_if<bir::PhiInst>(&inst)) {
        append_direct_global_address_materialization(
            names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, phi->result);
      } else if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (call->result.has_value()) {
          append_direct_global_address_materialization(
              names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, *call->result);
        }
        for (const auto& argument : call->args) {
          append_call_argument_address_materialization(names,
                                                       function_addressing,
                                                       notes,
                                                       module,
                                                       target_profile,
                                                       function_name_id,
                                                       block_label_id,
                                                       inst_index,
                                                       argument);
        }
      } else if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
        append_direct_global_address_materialization(
            names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, load_local->result);
        append_string_constant_address_materialization(
            names,
            function_addressing,
            notes,
            module,
            function_name_id,
            block_label_id,
            inst_index,
            load_local->result,
            load_local->address,
            static_cast<std::int64_t>(load_local->byte_offset));
        append_label_address_materialization(
            names,
            function_addressing,
            notes,
            module,
            function,
            function_name_id,
            block_label_id,
            inst_index,
            load_local->result,
            load_local->address,
            static_cast<std::int64_t>(load_local->byte_offset));
      } else if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
        append_direct_global_address_materialization(
            names, function_addressing, notes, module, target_profile, function_name_id, block_label_id, inst_index, load_global->result);
        append_string_constant_address_materialization(
            names,
            function_addressing,
            notes,
            module,
            function_name_id,
            block_label_id,
            inst_index,
            load_global->result,
            load_global->address,
            static_cast<std::int64_t>(load_global->byte_offset));
        append_label_address_materialization(
            names,
            function_addressing,
            notes,
            module,
            function,
            function_name_id,
            block_label_id,
            inst_index,
            load_global->result,
            load_global->address,
            static_cast<std::int64_t>(load_global->byte_offset));
      }
    }
  }
}

}  // namespace

void BirPreAlloc::run_stack_layout() {
  prepared_.completed_phases.push_back("stack_layout");
  prepared_.stack_layout = {};
  prepared_.addressing.functions.clear();

  PreparedObjectId next_object_id = 0;
  PreparedFrameSlotId next_slot_id = 0;

  for (const auto& function : prepared_.module.functions) {
    if (function.is_declaration) {
      continue;
    }

    auto function_objects =
        stack_layout::collect_function_stack_objects(
            prepared_.names, prepared_.module.names, function, next_object_id);
    stack_layout::apply_alloca_coalescing_hints(prepared_.names, function, function_objects);
    stack_layout::apply_copy_coalescing_hints(prepared_.names, function, function_objects);

    const auto inline_asm_summary = stack_layout::summarize_inline_asm(function);
    stack_layout::apply_regalloc_hints(
        prepared_.names, function, inline_asm_summary, function_objects);

    std::size_t function_frame_size = 0;
    std::size_t function_frame_alignment = 1;
    auto function_slots = stack_layout::assign_frame_slots(
        prepared_.names, function_objects, next_slot_id, function_frame_size, function_frame_alignment);

    prepared_.stack_layout.frame_size_bytes =
        std::max(prepared_.stack_layout.frame_size_bytes, function_frame_size);
    prepared_.stack_layout.frame_alignment_bytes =
        std::max(prepared_.stack_layout.frame_alignment_bytes, function_frame_alignment);
    const FunctionNameId function_name_id = prepared_.names.function_names.intern(function.name);
    auto& function_addressing = prepared_.addressing.functions.emplace_back(PreparedAddressingFunction{
        .function_name = function_name_id,
        .frame_size_bytes = function_frame_size,
        .frame_alignment_bytes = function_frame_alignment,
    });
    append_direct_frame_slot_accesses(
        prepared_.names,
        function_addressing,
        function_name_id,
        prepared_.module.names,
        function,
        build_frame_slot_map(function_objects, function_slots));
    append_address_materializations(prepared_.names,
                                    function_addressing,
                                    prepared_.notes,
                                    prepared_.module,
                                    prepared_.target_profile,
                                    function_name_id,
                                    function);

    prepared_.stack_layout.objects.insert(prepared_.stack_layout.objects.end(),
                                          std::make_move_iterator(function_objects.begin()),
                                          std::make_move_iterator(function_objects.end()));
    prepared_.stack_layout.frame_slots.insert(prepared_.stack_layout.frame_slots.end(),
                                              std::make_move_iterator(function_slots.begin()),
                                              std::make_move_iterator(function_slots.end()));

    prepared_.notes.push_back(PrepareNote{
        .phase = "stack_layout",
        .message = "stack layout prepared function '" + function.name + "' with " +
                   std::to_string(function_slots.size()) + " home slot(s) and " +
                   std::to_string(function_frame_size) + " bytes of frame space",
    });
    if (inline_asm_summary.instruction_count != 0) {
      prepared_.notes.push_back(PrepareNote{
          .phase = "stack_layout",
          .message = "stack layout observed " +
                     std::to_string(inline_asm_summary.instruction_count) +
                     " inline asm instruction(s) in '" + function.name + "'",
      });
    }
  }

  prepared_.notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message = "stack layout now emits provisional stack objects, elides dead and copy-"
                 "coalesced home slots, and only allocates frame storage for objects that "
                 "still require dedicated homes",
  });
}

}  // namespace c4c::backend::prepare
