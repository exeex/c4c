#include "support.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare::stack_layout {

namespace {

[[nodiscard]] bool uses_copy_coalesced_slot(const PreparedStackObject& object) {
  return object.source_kind == std::string_view("copy_coalescing_candidate");
}

[[nodiscard]] bool uses_fixed_location_slot(const PreparedStackObject& object) {
  return object.permanent_home_slot;
}

[[nodiscard]] bool is_parameter_owned_fixed_slot(const PreparedStackObject& object) {
  return object.source_kind == std::string_view("byval_param") ||
         object.source_kind == std::string_view("sret_param");
}

void sort_reorderable_objects(std::vector<const PreparedStackObject*>& objects) {
  std::stable_sort(objects.begin(),
                   objects.end(),
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
}

[[nodiscard]] std::size_t aligned_end_offset(std::size_t offset_bytes,
                                             const PreparedStackObject& object) {
  const std::size_t size_bytes = normalize_size(object.type, object.size_bytes);
  const std::size_t align_bytes =
      normalize_alignment(object.type, object.align_bytes, size_bytes);
  return align_to(offset_bytes, align_bytes) + size_bytes;
}

std::vector<const PreparedStackObject*>::iterator select_gap_filler(
    std::vector<const PreparedStackObject*>& objects,
    const std::size_t next_offset_bytes) {
  if (objects.size() < 2) {
    return objects.end();
  }

  const std::size_t primary_start = align_to(
      next_offset_bytes,
      normalize_alignment(objects.front()->type,
                          objects.front()->align_bytes,
                          normalize_size(objects.front()->type, objects.front()->size_bytes)));
  if (primary_start == next_offset_bytes) {
    return objects.end();
  }

  auto best_it = objects.end();
  std::size_t best_end_offset = next_offset_bytes;
  for (auto it = std::next(objects.begin()); it != objects.end(); ++it) {
    const std::size_t end_offset = aligned_end_offset(next_offset_bytes, **it);
    if (end_offset > primary_start) {
      continue;
    }
    if (best_it == objects.end() || end_offset > best_end_offset ||
        (end_offset == best_end_offset && (*it)->object_id < (*best_it)->object_id)) {
      best_it = it;
      best_end_offset = end_offset;
    }
  }

  return best_it;
}

void append_slot_for_object(const PreparedStackObject& object,
                            bool fixed_location,
                            PreparedFrameSlotId& next_slot_id,
                            std::size_t& next_offset_bytes,
                            std::size_t& max_alignment_bytes,
                            std::vector<PreparedFrameSlot>& slots) {
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
      .fixed_location = fixed_location,
  });

  next_offset_bytes += size_bytes;
}

}  // namespace

std::vector<PreparedFrameSlot> assign_frame_slots(const std::vector<PreparedStackObject>& objects,
                                                  PreparedFrameSlotId& next_slot_id,
                                                  std::size_t& frame_size_bytes,
                                                  std::size_t& frame_alignment_bytes) {
  std::vector<PreparedFrameSlot> slots;
  slots.reserve(objects.size());

  std::vector<const PreparedStackObject*> fixed_location_objects;
  std::vector<const PreparedStackObject*> parameter_fixed_location_objects;
  std::vector<const PreparedStackObject*> local_fixed_location_objects;
  std::vector<const PreparedStackObject*> reorderable_objects;
  fixed_location_objects.reserve(objects.size());
  parameter_fixed_location_objects.reserve(objects.size());
  local_fixed_location_objects.reserve(objects.size());
  reorderable_objects.reserve(objects.size());

  for (const auto& object : objects) {
    if (!object.requires_home_slot || uses_copy_coalesced_slot(object)) {
      continue;
    }
    if (uses_fixed_location_slot(object)) {
      if (is_parameter_owned_fixed_slot(object)) {
        parameter_fixed_location_objects.push_back(&object);
      } else {
        local_fixed_location_objects.push_back(&object);
      }
      continue;
    }
    reorderable_objects.push_back(&object);
  }

  fixed_location_objects.insert(fixed_location_objects.end(),
                                parameter_fixed_location_objects.begin(),
                                parameter_fixed_location_objects.end());
  fixed_location_objects.insert(fixed_location_objects.end(),
                                local_fixed_location_objects.begin(),
                                local_fixed_location_objects.end());

  sort_reorderable_objects(reorderable_objects);

  std::size_t next_offset_bytes = 0;
  std::size_t max_alignment_bytes = 1;

  for (const PreparedStackObject* object : fixed_location_objects) {
    append_slot_for_object(*object, true, next_slot_id, next_offset_bytes, max_alignment_bytes, slots);
  }

  while (!reorderable_objects.empty()) {
    auto selected_it = select_gap_filler(reorderable_objects, next_offset_bytes);
    if (selected_it == reorderable_objects.end()) {
      selected_it = reorderable_objects.begin();
    }

    const PreparedStackObject* object = *selected_it;
    append_slot_for_object(*object, false, next_slot_id, next_offset_bytes, max_alignment_bytes, slots);
    reorderable_objects.erase(selected_it);
  }

  frame_alignment_bytes = max_alignment_bytes;
  frame_size_bytes = align_to(next_offset_bytes, frame_alignment_bytes);
  return slots;
}

}  // namespace c4c::backend::prepare::stack_layout
