#include "stack_layout.hpp"

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/stack_layout/ for the intended
// frame, slot-assignment, and related analysis shape.

namespace c4c::backend::prepare {

void run_stack_layout(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("stack_layout");
  module.notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message = "stack layout skeleton is wired; no frame/slot allocation is performed yet",
  });
}

void run_stack_layout(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("stack_layout");
  module.stack_layout.objects.clear();
  for (const auto& function : module.module.functions) {
    for (const auto& slot : function.local_slots) {
      module.stack_layout.objects.push_back(PreparedStackObject{
          .function_name = function.name,
          .source_name = slot.name,
          .source_kind = "local_slot",
          .type = slot.type,
          .size_bytes = slot.size_bytes,
          .align_bytes = slot.align_bytes,
      });
    }
  }
  module.notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message =
          "stack layout now publishes local-slot stack objects as prepared artifacts; frame "
          "offset assignment remains future work",
  });
}

}  // namespace c4c::backend::prepare
