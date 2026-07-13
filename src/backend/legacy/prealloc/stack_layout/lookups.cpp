#include "stack_layout.hpp"

namespace c4c::backend::prepare {

[[nodiscard]] const PreparedFrameSlot* find_frame_slot_by_id(
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedStackObject* find_stack_object_by_id(
    const PreparedStackLayout& stack_layout,
    PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

}  // namespace c4c::backend::prepare
