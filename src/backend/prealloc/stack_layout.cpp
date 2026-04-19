#include "prealloc.hpp"
#include "stack_layout/support.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace c4c::backend::prepare {

namespace {

using FrameSlotMap =
    std::unordered_map<std::string_view, const PreparedFrameSlot*>;

[[nodiscard]] FrameSlotMap build_frame_slot_map(
    const std::vector<PreparedStackObject>& function_objects,
    const std::vector<PreparedFrameSlot>& function_slots) {
  std::unordered_map<PreparedObjectId, std::string_view> slot_names_by_object_id;
  slot_names_by_object_id.reserve(function_objects.size());
  for (const auto& object : function_objects) {
    slot_names_by_object_id.emplace(object.object_id, object.source_name);
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

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_frame_slot_access(
    std::string_view function_name,
    std::string_view block_label,
    std::size_t inst_index,
    const bir::LoadLocalInst& inst,
    const FrameSlotMap& frame_slots_by_name) {
  if (inst.address.has_value() &&
      inst.address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const auto slot_it = frame_slots_by_name.find(inst.slot_name);
  if (slot_it == frame_slots_by_name.end()) {
    return std::nullopt;
  }

  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.result.type, inst.result.type == bir::TypeKind::Void
                                                         ? 0
                                                         : stack_layout::fallback_type_size(
                                                               inst.result.type));
  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .result_value_name = inst.result.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.result.name)
                               : std::nullopt,
      .address =
          PreparedAddress{
              .base_kind = PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = slot_it->second->slot_id,
              .byte_offset = static_cast<std::int64_t>(inst.byte_offset),
              .size_bytes = size_bytes,
              .align_bytes = stack_layout::normalize_alignment(
                  inst.result.type, inst.align_bytes, size_bytes),
              .can_use_base_plus_offset = true,
          },
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_frame_slot_access(
    std::string_view function_name,
    std::string_view block_label,
    std::size_t inst_index,
    const bir::StoreLocalInst& inst,
    const FrameSlotMap& frame_slots_by_name) {
  if (inst.address.has_value() &&
      inst.address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const auto slot_it = frame_slots_by_name.find(inst.slot_name);
  if (slot_it == frame_slots_by_name.end()) {
    return std::nullopt;
  }

  const std::size_t size_bytes =
      stack_layout::normalize_size(inst.value.type, inst.value.type == bir::TypeKind::Void
                                                        ? 0
                                                        : stack_layout::fallback_type_size(
                                                              inst.value.type));
  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .stored_value_name = inst.value.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.value.name)
                               : std::nullopt,
      .address =
          PreparedAddress{
              .base_kind = PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = slot_it->second->slot_id,
              .byte_offset = static_cast<std::int64_t>(inst.byte_offset),
              .size_bytes = size_bytes,
              .align_bytes = stack_layout::normalize_alignment(
                  inst.value.type, inst.align_bytes, size_bytes),
              .can_use_base_plus_offset = true,
          },
  };
}

[[nodiscard]] std::optional<PreparedAddress> build_direct_symbol_backed_address(
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
        .symbol_name = std::string(fallback_symbol_name),
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
      .symbol_name = std::string(symbol_name),
      .byte_offset = address->byte_offset + fallback_byte_offset,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes == 0 ? address->align_bytes : align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    std::string_view function_name,
    std::string_view block_label,
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
      inst.address, inst.global_name, static_cast<std::int64_t>(inst.byte_offset), size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .result_value_name = inst.result.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.result.name)
                               : std::nullopt,
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_direct_symbol_backed_access(
    std::string_view function_name,
    std::string_view block_label,
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
      inst.address, inst.global_name, static_cast<std::int64_t>(inst.byte_offset), size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .stored_value_name = inst.value.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.value.name)
                               : std::nullopt,
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedAddress> build_pointer_indirect_address(
    const std::optional<bir::MemoryAddress>& address,
    std::size_t size_bytes,
    std::size_t align_bytes) {
  if (!address.has_value() || address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      address->base_value.kind != bir::Value::Kind::Named || address->base_value.name.empty()) {
    return std::nullopt;
  }

  return PreparedAddress{
      .base_kind = PreparedAddressBaseKind::PointerValue,
      .pointer_value_name = address->base_value.name,
      .byte_offset = address->byte_offset,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes == 0 ? address->align_bytes : align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    std::string_view function_name,
    std::string_view block_label,
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
  auto address = build_pointer_indirect_address(inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .result_value_name = inst.result.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.result.name)
                               : std::nullopt,
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    std::string_view function_name,
    std::string_view block_label,
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
  auto address = build_pointer_indirect_address(inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .stored_value_name = inst.value.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.value.name)
                               : std::nullopt,
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    std::string_view function_name,
    std::string_view block_label,
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
  auto address = build_pointer_indirect_address(inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .result_value_name = inst.result.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.result.name)
                               : std::nullopt,
      .address = std::move(*address),
  };
}

[[nodiscard]] std::optional<PreparedMemoryAccess> build_pointer_indirect_access(
    std::string_view function_name,
    std::string_view block_label,
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
  auto address = build_pointer_indirect_address(inst.address, size_bytes, align_bytes);
  if (!address.has_value()) {
    return std::nullopt;
  }

  return PreparedMemoryAccess{
      .function_name = std::string(function_name),
      .block_label = std::string(block_label),
      .inst_index = inst_index,
      .stored_value_name = inst.value.kind == bir::Value::Kind::Named
                               ? std::optional<std::string>(inst.value.name)
                               : std::nullopt,
      .address = std::move(*address),
  };
}

void append_direct_frame_slot_accesses(PreparedAddressingFunction& function_addressing,
                                       const bir::Function& function,
                                       const FrameSlotMap& frame_slots_by_name) {
  for (const auto& block : function.blocks) {
    for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
      const auto& inst = block.insts[inst_index];
      if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
        if (auto access =
                build_pointer_indirect_access(function.name, block.label, inst_index, *load_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_frame_slot_access(
                function.name, block.label, inst_index, *load_local, frame_slots_by_name);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
        continue;
      }
      if (const auto* store_local = std::get_if<bir::StoreLocalInst>(&inst)) {
        if (auto access =
                build_pointer_indirect_access(function.name, block.label, inst_index, *store_local);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_frame_slot_access(
                function.name, block.label, inst_index, *store_local, frame_slots_by_name);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
        continue;
      }
      if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
        if (auto access =
                build_pointer_indirect_access(function.name, block.label, inst_index, *load_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_symbol_backed_access(
                function.name, block.label, inst_index, *load_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
        }
        continue;
      }
      if (const auto* store_global = std::get_if<bir::StoreGlobalInst>(&inst)) {
        if (auto access = build_pointer_indirect_access(
                function.name, block.label, inst_index, *store_global);
            access.has_value()) {
          function_addressing.accesses.push_back(std::move(*access));
          continue;
        }
        if (auto access = build_direct_symbol_backed_access(
                function.name, block.label, inst_index, *store_global);
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

    auto function_objects = stack_layout::collect_function_stack_objects(function, next_object_id);
    stack_layout::apply_alloca_coalescing_hints(function, function_objects);
    stack_layout::apply_copy_coalescing_hints(function, function_objects);

    const auto inline_asm_summary = stack_layout::summarize_inline_asm(function);
    stack_layout::apply_regalloc_hints(function, inline_asm_summary, function_objects);

    std::size_t function_frame_size = 0;
    std::size_t function_frame_alignment = 1;
    auto function_slots = stack_layout::assign_frame_slots(
        function_objects, next_slot_id, function_frame_size, function_frame_alignment);

    prepared_.stack_layout.frame_size_bytes =
        std::max(prepared_.stack_layout.frame_size_bytes, function_frame_size);
    prepared_.stack_layout.frame_alignment_bytes =
        std::max(prepared_.stack_layout.frame_alignment_bytes, function_frame_alignment);
    auto& function_addressing = prepared_.addressing.functions.emplace_back(PreparedAddressingFunction{
        .function_name = function.name,
        .frame_size_bytes = function_frame_size,
        .frame_alignment_bytes = function_frame_alignment,
    });
    append_direct_frame_slot_accesses(
        function_addressing, function, build_frame_slot_map(function_objects, function_slots));

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
