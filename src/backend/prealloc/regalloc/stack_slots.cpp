#include "stack_slots.hpp"

#include "../stack_layout/stack_layout.hpp"

#include <algorithm>
#include <unordered_set>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

[[nodiscard]] PreparedObjectId next_stack_object_id(const PreparedStackLayout& stack_layout) {
  PreparedObjectId next = 0;
  for (const auto& object : stack_layout.objects) {
    next = std::max(next, object.object_id + 1U);
  }
  return next;
}

}  // namespace

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

PreparedFrameSlotId next_frame_slot_id(const PreparedStackLayout& stack_layout) {
  PreparedFrameSlotId next = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    next = std::max(next, slot.slot_id + 1U);
  }
  return next;
}

std::size_t function_frame_extent(const PreparedStackLayout& stack_layout,
                                  FunctionNameId function_name) {
  std::size_t extent = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name == function_name) {
      extent = std::max(extent, slot.offset_bytes + slot.size_bytes);
    }
  }
  return extent;
}

std::size_t function_frame_alignment(const PreparedStackLayout& stack_layout,
                                     FunctionNameId function_name) {
  std::size_t alignment = 1;
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name == function_name) {
      alignment = std::max(alignment, slot.align_bytes);
    }
  }
  return alignment;
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

void publish_regalloc_stack_slots(PreparedStackLayout& stack_layout,
                                  const PreparedRegallocFunction& function) {
  PreparedObjectId next_object_id = next_stack_object_id(stack_layout);
  std::unordered_set<PreparedFrameSlotId> existing_slot_ids;
  existing_slot_ids.reserve(stack_layout.frame_slots.size() + function.values.size());
  for (const auto& slot : stack_layout.frame_slots) {
    existing_slot_ids.insert(slot.slot_id);
  }
  std::size_t new_slot_count = 0;
  for (const auto& value : function.values) {
    if (value.assigned_stack_slot.has_value() &&
        existing_slot_ids.find(value.assigned_stack_slot->slot_id) == existing_slot_ids.end()) {
      ++new_slot_count;
    }
  }
  stack_layout.objects.reserve(stack_layout.objects.size() + new_slot_count);
  stack_layout.frame_slots.reserve(stack_layout.frame_slots.size() + new_slot_count);
  for (const auto& value : function.values) {
    if (!value.assigned_stack_slot.has_value() ||
        !existing_slot_ids.insert(value.assigned_stack_slot->slot_id).second) {
      continue;
    }
    const PreparedObjectId object_id = next_object_id++;
    const std::size_t size_bytes = value.assigned_stack_slot->size_bytes.value_or(0);
    const std::size_t align_bytes = value.assigned_stack_slot->align_bytes.value_or(1);
    stack_layout.objects.push_back(PreparedStackObject{
        .object_id = object_id,
        .function_name = function.function_name,
        .slot_name = std::nullopt,
        .value_name = value.value_name,
        .source_kind =
            value.requires_home_slot ? "regalloc.home_slot" : "regalloc.spill_slot",
        .type = value.type,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .address_exposed = false,
        .requires_home_slot = value.requires_home_slot,
        .permanent_home_slot = value.requires_home_slot,
    });
    stack_layout.frame_slots.push_back(PreparedFrameSlot{
        .slot_id = value.assigned_stack_slot->slot_id,
        .object_id = object_id,
        .function_name = function.function_name,
        .offset_bytes = value.assigned_stack_slot->offset_bytes,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .fixed_location = false,
    });
  }
}

}  // namespace c4c::backend::prepare::regalloc_detail
