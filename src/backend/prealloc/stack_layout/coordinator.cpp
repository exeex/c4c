#include "../prealloc.hpp"
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
    std::string_view slot_name,
    const std::optional<bir::MemoryAddress>& address,
    std::int64_t access_byte_offset,
    std::size_t access_size_bytes,
    const FrameSlotMap& frame_slots_by_name) {
  std::string_view requested_slot_name = slot_name;
  if (address.has_value()) {
    if (address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot ||
        address->base_name.empty()) {
      return {};
    }
    requested_slot_name = address->base_name;
  }

  ResolvedFrameSlot resolved;
  if (const auto slot_it = frame_slots_by_name.find(names.slot_names.find(requested_slot_name));
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
          names, inst.slot_name, inst.address, access_byte_offset, size_bytes, frame_slots_by_name);
  if (resolved_frame_slot.frame_slot == nullptr) {
    return std::nullopt;
  }
  const auto byte_offset = access_byte_offset + resolved_frame_slot.byte_offset_adjust;
  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
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
          names, inst.slot_name, inst.address, access_byte_offset, size_bytes, frame_slots_by_name);
  if (resolved_frame_slot.frame_slot == nullptr) {
    return std::nullopt;
  }
  const auto byte_offset = access_byte_offset + resolved_frame_slot.byte_offset_adjust;
  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
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
    const std::optional<bir::MemoryAddress>& address,
    std::string_view fallback_symbol_name,
    std::int64_t fallback_byte_offset,
    std::size_t size_bytes,
    std::size_t align_bytes) {
  if (!address.has_value()) {
    if (fallback_symbol_name.empty()) {
      return std::nullopt;
    }
    return PreparedAddress{
        .base_kind = PreparedAddressBaseKind::GlobalSymbol,
        .symbol_name = names.link_names.intern(fallback_symbol_name),
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
  if (symbol_name.empty()) {
    return std::nullopt;
  }

  return PreparedAddress{
      .base_kind = base_kind,
      .symbol_name = names.link_names.intern(symbol_name),
      .byte_offset = address->byte_offset + fallback_byte_offset,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes == 0 ? address->align_bytes : align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
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
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes : inst.align_bytes,
      size_bytes);
  auto address = build_direct_symbol_backed_address(
      names, inst.address, {}, static_cast<std::int64_t>(inst.byte_offset), size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .result_value_name = prepared_named_value_id(names, inst.result),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
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
      inst.align_bytes == 0 && inst.address.has_value() ? inst.address->align_bytes : inst.align_bytes,
      size_bytes);
  auto address = build_direct_symbol_backed_address(
      names, inst.address, {}, static_cast<std::int64_t>(inst.byte_offset), size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = function_name_id,
      .block_label = block_label_id,
      .inst_index = inst_index,
      .stored_value_name = prepared_named_value_id(names, inst.value),
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    PreparedNameTables& names,
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
      inst.address,
      inst.global_name,
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
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    PreparedNameTables& names,
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
      inst.address,
      inst.global_name,
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
                names, function_name_id, block_label_id, inst_index, *load_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_frame_slot_access(
                names, function_name_id, block_label_id, inst_index, *load_local, frame_slots_by_name);
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
                names, function_name_id, block_label_id, inst_index, *store_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_frame_slot_access(
                names, function_name_id, block_label_id, inst_index, *store_local, frame_slots_by_name);
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
                names, function_name_id, block_label_id, inst_index, *load_global);
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
                names, function_name_id, block_label_id, inst_index, *store_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
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
        stack_layout::collect_function_stack_objects(prepared_.names, function, next_object_id);
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
