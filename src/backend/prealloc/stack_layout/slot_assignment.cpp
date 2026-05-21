#include "../module.hpp"
#include "stack_layout.hpp"

#include <algorithm>
#include <optional>
#include <unordered_map>
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

struct SliceFamilyLayout {
  bool fixed_location = false;
  bool preserve_slice_offsets = false;
  std::size_t max_alignment_bytes = 1;
  std::optional<std::size_t> base_offset_bytes;
};

struct ObjectSliceLayout {
  std::string family_name;
  std::size_t slice_offset = 0;
  SliceFamilyLayout* family = nullptr;
};

[[nodiscard]] std::unordered_map<std::string, SliceFamilyLayout> build_slice_family_layout_map(
    const PreparedNameTables& names,
    const std::vector<PreparedStackObject>& objects) {
  struct SliceEntry {
    std::size_t ordinal = 0;
    std::size_t size_bytes = 0;
    std::size_t align_bytes = 1;
  };

  std::unordered_map<std::string, SliceFamilyLayout> family_layouts;
  std::unordered_map<std::string, std::vector<SliceEntry>> family_entries;
  for (const auto& object : objects) {
    if (uses_copy_coalesced_slot(object)) {
      continue;
    }
    if (!object.slot_name.has_value()) {
      continue;
    }
    const auto slice = parse_prepared_slot_slice_name(prepared_slot_name(names, *object.slot_name));
    if (!slice.has_value()) {
      continue;
    }
    const std::string family_name(slice->first);
    auto& layout = family_layouts[family_name];
    layout.fixed_location =
        layout.fixed_location || (object.requires_home_slot && uses_fixed_location_slot(object));
    const std::size_t size_bytes = normalize_size(object.type, object.size_bytes);
    const std::size_t align_bytes =
        normalize_alignment(object.type, object.align_bytes, size_bytes);
    layout.max_alignment_bytes = std::max(layout.max_alignment_bytes, align_bytes);
    family_entries[family_name].push_back(SliceEntry{
        .ordinal = slice->second,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
    });
  }

  for (auto& [family_name, entries] : family_entries) {
    std::sort(entries.begin(),
              entries.end(),
              [](const SliceEntry& lhs, const SliceEntry& rhs) {
                if (lhs.ordinal != rhs.ordinal) {
                  return lhs.ordinal < rhs.ordinal;
                }
                if (lhs.size_bytes != rhs.size_bytes) {
                  return lhs.size_bytes < rhs.size_bytes;
                }
                return lhs.align_bytes < rhs.align_bytes;
              });

    bool dense_ordinal_family = !entries.empty();
    bool alignment_would_pad_slice_offsets = false;
    for (std::size_t index = 0; index < entries.size(); ++index) {
      if (entries[index].align_bytes > entries[index].size_bytes) {
        alignment_would_pad_slice_offsets = true;
      }
      if (entries[index].ordinal != index ||
          (index > 0 && entries[index - 1].ordinal == entries[index].ordinal)) {
        dense_ordinal_family = false;
        break;
      }
    }
    family_layouts[family_name].preserve_slice_offsets =
        !dense_ordinal_family || alignment_would_pad_slice_offsets;
  }

  return family_layouts;
}

[[nodiscard]] bool belongs_to_fixed_slice_family(
    const PreparedNameTables& names,
    const PreparedStackObject& object,
    const std::unordered_map<std::string, SliceFamilyLayout>& family_layouts) {
  if (!object.slot_name.has_value()) {
    return false;
  }
  const auto slice = parse_prepared_slot_slice_name(prepared_slot_name(names, *object.slot_name));
  if (!slice.has_value()) {
    return false;
  }
  const auto family_it = family_layouts.find(std::string(slice->first));
  return family_it != family_layouts.end() && family_it->second.fixed_location;
}

[[nodiscard]] std::optional<ObjectSliceLayout> find_object_slice_layout(
    const PreparedNameTables& names,
    const PreparedStackObject& object,
    std::unordered_map<std::string, SliceFamilyLayout>& family_layouts) {
  if (!object.slot_name.has_value()) {
    return std::nullopt;
  }
  const auto slice = parse_prepared_slot_slice_name(prepared_slot_name(names, *object.slot_name));
  if (!slice.has_value()) {
    return std::nullopt;
  }
  auto family_it = family_layouts.find(std::string(slice->first));
  if (family_it == family_layouts.end() || !family_it->second.fixed_location ||
      !family_it->second.preserve_slice_offsets) {
    return std::nullopt;
  }
  return ObjectSliceLayout{
      .family_name = std::string(slice->first),
      .slice_offset = slice->second,
      .family = &family_it->second,
  };
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

void append_slice_offset_slot_for_object(const PreparedStackObject& object,
                                         ObjectSliceLayout& slice,
                                         bool fixed_location,
                                         PreparedFrameSlotId& next_slot_id,
                                         std::size_t& next_offset_bytes,
                                         std::size_t& max_alignment_bytes,
                                         std::vector<PreparedFrameSlot>& slots) {
  const std::size_t size_bytes = normalize_size(object.type, object.size_bytes);
  const std::size_t align_bytes =
      normalize_alignment(object.type, object.align_bytes, size_bytes);

  if (!slice.family->base_offset_bytes.has_value()) {
    slice.family->base_offset_bytes =
        align_to(next_offset_bytes, slice.family->max_alignment_bytes);
  }

  const std::size_t offset_bytes = *slice.family->base_offset_bytes + slice.slice_offset;
  max_alignment_bytes = std::max(max_alignment_bytes, slice.family->max_alignment_bytes);
  max_alignment_bytes = std::max(max_alignment_bytes, align_bytes);

  slots.push_back(PreparedFrameSlot{
      .slot_id = next_slot_id++,
      .object_id = object.object_id,
      .function_name = object.function_name,
      .offset_bytes = offset_bytes,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .fixed_location = fixed_location,
  });

  next_offset_bytes = std::max(next_offset_bytes, offset_bytes + size_bytes);
}

}  // namespace

std::vector<PreparedFrameSlot> assign_frame_slots(const PreparedNameTables& names,
                                                  const std::vector<PreparedStackObject>& objects,
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
  auto slice_family_layouts = build_slice_family_layout_map(names, objects);

  for (const auto& object : objects) {
    if (uses_copy_coalesced_slot(object)) {
      continue;
    }
    const bool fixed_location =
        uses_fixed_location_slot(object) ||
        belongs_to_fixed_slice_family(names, object, slice_family_layouts);
    if (!object.requires_home_slot && !fixed_location) {
      continue;
    }
    if (fixed_location) {
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
    if (auto slice = find_object_slice_layout(names, *object, slice_family_layouts);
        slice.has_value()) {
      append_slice_offset_slot_for_object(
          *object, *slice, true, next_slot_id, next_offset_bytes, max_alignment_bytes, slots);
    } else {
      append_slot_for_object(
          *object, true, next_slot_id, next_offset_bytes, max_alignment_bytes, slots);
    }
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
