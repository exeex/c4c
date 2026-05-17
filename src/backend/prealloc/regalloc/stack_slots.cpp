#include "stack_slots.hpp"

#include "../stack_layout/stack_layout.hpp"

#include <algorithm>

namespace c4c::backend::prepare::regalloc_detail {

std::size_t normalized_value_size(const PreparedRegallocValue& value) {
  return stack_layout::normalize_size(value.type, 0);
}

std::size_t normalized_value_alignment(const PreparedRegallocValue& value) {
  return stack_layout::normalize_alignment(value.type, 0, normalized_value_size(value));
}

std::optional<PreparedStackSlotAssignment> existing_stack_slot_assignment(
    const PreparedStackLayout& stack_layout,
    const PreparedRegallocValue& value) {
  if (!value.stack_object_id.has_value()) {
    return std::nullopt;
  }
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.object_id != *value.stack_object_id) {
      continue;
    }
    return PreparedStackSlotAssignment{
        .slot_id = slot.slot_id,
        .offset_bytes = slot.offset_bytes,
        .size_bytes = slot.size_bytes,
        .align_bytes = slot.align_bytes,
    };
  }
  return std::nullopt;
}

PreparedStackSlotAssignment allocate_stack_slot(const PreparedRegallocValue& value,
                                                const PreparedStackLayout& stack_layout,
                                                PreparedFrameSlotId& next_slot_id,
                                                std::size_t& next_offset_bytes,
                                                std::size_t& frame_alignment_bytes) {
  if (const auto existing = existing_stack_slot_assignment(stack_layout, value); existing.has_value()) {
    return *existing;
  }

  const std::size_t size_bytes = normalized_value_size(value);
  const std::size_t align_bytes = normalized_value_alignment(value);
  next_offset_bytes = stack_layout::align_to(next_offset_bytes, align_bytes);
  PreparedStackSlotAssignment slot{
      .slot_id = next_slot_id++,
      .offset_bytes = next_offset_bytes,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
  };
  next_offset_bytes += size_bytes;
  frame_alignment_bytes = std::max(frame_alignment_bytes, align_bytes);
  return slot;
}

}  // namespace c4c::backend::prepare::regalloc_detail
