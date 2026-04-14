#include "stack_layout.hpp"

namespace c4c::backend::prepare {

void run_stack_layout(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("stack_layout");
  module.notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message = "stack layout skeleton is wired; no frame/slot allocation is performed yet",
  });
}

}  // namespace c4c::backend::prepare
