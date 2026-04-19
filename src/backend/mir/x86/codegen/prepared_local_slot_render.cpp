#include "x86_codegen.hpp"

#include <algorithm>

namespace c4c::backend::x86 {

namespace {

std::size_t align_up(std::size_t value, std::size_t align) {
  if (align <= 1) {
    return value;
  }
  const auto remainder = value % align;
  return remainder == 0 ? value : value + (align - remainder);
}

}  // namespace

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    c4c::TargetArch prepared_arch) {
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  PreparedModuleLocalSlotLayout layout;
  std::size_t next_offset = 0;
  std::size_t max_align = 16;
  for (const auto& slot : function.local_slots) {
    if (slot.type != c4c::backend::bir::TypeKind::I8 &&
        slot.type != c4c::backend::bir::TypeKind::I16 &&
        slot.type != c4c::backend::bir::TypeKind::I32 &&
        slot.type != c4c::backend::bir::TypeKind::I64 &&
        slot.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto slot_size = slot.size_bytes != 0
                               ? slot.size_bytes
                               : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                  : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                                                                  : 1u);
    const auto slot_align = slot.align_bytes != 0 ? slot.align_bytes : slot_size;
    if (slot_size == 0 || slot_size > 8 || slot_align > 16) {
      return std::nullopt;
    }
    next_offset = align_up(next_offset, slot_align);
    layout.offsets.emplace(slot.name, next_offset);
    next_offset += slot_size;
    max_align = std::max(max_align, slot_align);
  }
  layout.frame_size = align_up(next_offset, max_align);
  return layout;
}

std::string render_prepared_stack_memory_operand(std::size_t byte_offset,
                                                 std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

std::string render_prepared_stack_address_expr(std::size_t byte_offset) {
  if (byte_offset == 0) {
    return "[rsp]";
  }
  return "[rsp + " + std::to_string(byte_offset) + "]";
}

std::optional<std::string> render_prepared_local_address_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const std::optional<c4c::backend::bir::MemoryAddress>& address,
    std::string_view size_name) {
  if (!address.has_value() ||
      address->base_kind != c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const auto slot_it = local_layout.offsets.find(address->base_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  const auto signed_byte_offset =
      static_cast<std::int64_t>(slot_it->second) + address->byte_offset;
  if (signed_byte_offset < 0) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset),
                                              size_name);
}

std::optional<std::string> render_prepared_local_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view slot_name,
    std::size_t stack_byte_bias,
    std::string_view size_name) {
  const auto slot_it = local_layout.offsets.find(slot_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(slot_it->second + stack_byte_bias, size_name);
}

}  // namespace c4c::backend::x86
