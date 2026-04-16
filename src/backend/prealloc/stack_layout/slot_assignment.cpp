#include "support.hpp"

#include <algorithm>
#include <vector>

namespace c4c::backend::prepare::stack_layout {

std::vector<PreparedFrameSlot> assign_frame_slots(const std::vector<PreparedStackObject>& objects,
                                                  PreparedFrameSlotId& next_slot_id,
                                                  std::size_t& frame_size_bytes,
                                                  std::size_t& frame_alignment_bytes) {
  std::vector<PreparedFrameSlot> slots;
  slots.reserve(objects.size());

  std::size_t next_offset_bytes = 0;
  std::size_t max_alignment_bytes = 1;

  for (const auto& object : objects) {
    if (!object.requires_home_slot) {
      continue;
    }

    const std::size_t size_bytes = normalize_size(object.type, object.size_bytes);
    const std::size_t align_bytes =
        normalize_alignment(object.type, object.align_bytes, size_bytes);

    next_offset_bytes = align_to(next_offset_bytes, align_bytes);
    max_alignment_bytes = std::max(max_alignment_bytes, align_bytes);

    slots.push_back(PreparedFrameSlot{
        .slot_id = next_slot_id++,
        .object_id = object.object_id,
        .function_name = object.function_name,
        .offset_bytes = next_offset_bytes,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .fixed_location = false,
    });

    next_offset_bytes += size_bytes;
  }

  frame_alignment_bytes = max_alignment_bytes;
  frame_size_bytes = align_to(next_offset_bytes, frame_alignment_bytes);
  return slots;
}

}  // namespace c4c::backend::prepare::stack_layout
