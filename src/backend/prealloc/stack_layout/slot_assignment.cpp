#include "support.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare::stack_layout {

std::vector<PreparedFrameSlot> assign_frame_slots(const std::vector<PreparedStackObject>& objects,
                                                  PreparedFrameSlotId& next_slot_id,
                                                  std::size_t& frame_size_bytes,
                                                  std::size_t& frame_alignment_bytes) {
  std::vector<PreparedFrameSlot> slots;
  slots.reserve(objects.size());

  std::vector<const PreparedStackObject*> assignable_objects;
  assignable_objects.reserve(objects.size());

  for (const auto& object : objects) {
    const bool uses_copy_coalesced_slot =
        object.source_kind == std::string_view("copy_coalescing_candidate");
    if (!object.requires_home_slot || uses_copy_coalesced_slot) {
      continue;
    }
    assignable_objects.push_back(&object);
  }

  std::stable_sort(assignable_objects.begin(),
                   assignable_objects.end(),
                   [](const PreparedStackObject* lhs, const PreparedStackObject* rhs) {
                     const std::size_t lhs_size = normalize_size(lhs->type, lhs->size_bytes);
                     const std::size_t rhs_size = normalize_size(rhs->type, rhs->size_bytes);
                     const std::size_t lhs_align =
                         normalize_alignment(lhs->type, lhs->align_bytes, lhs_size);
                     const std::size_t rhs_align =
                         normalize_alignment(rhs->type, rhs->align_bytes, rhs_size);
                     if (lhs_align != rhs_align) {
                       return lhs_align > rhs_align;
                     }
                     if (lhs_size != rhs_size) {
                       return lhs_size > rhs_size;
                     }
                     return lhs->object_id < rhs->object_id;
                   });

  std::size_t next_offset_bytes = 0;
  std::size_t max_alignment_bytes = 1;

  for (const PreparedStackObject* object : assignable_objects) {
    const std::size_t size_bytes = normalize_size(object->type, object->size_bytes);
    const std::size_t align_bytes =
        normalize_alignment(object->type, object->align_bytes, size_bytes);

    next_offset_bytes = align_to(next_offset_bytes, align_bytes);
    max_alignment_bytes = std::max(max_alignment_bytes, align_bytes);

    slots.push_back(PreparedFrameSlot{
        .slot_id = next_slot_id++,
        .object_id = object->object_id,
        .function_name = object->function_name,
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
