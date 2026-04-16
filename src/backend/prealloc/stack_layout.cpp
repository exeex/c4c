#include "prealloc.hpp"
#include "stack_layout/support.hpp"

#include <algorithm>
#include <utility>

namespace c4c::backend::prepare {

void BirPreAlloc::run_stack_layout() {
  prepared_.completed_phases.push_back("stack_layout");
  prepared_.stack_layout = {};

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
