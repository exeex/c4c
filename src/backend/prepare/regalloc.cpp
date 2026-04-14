#include "regalloc.hpp"

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/regalloc.rs for the intended
// register-allocation pipeline and policy shape.

namespace c4c::backend::prepare {

void run_regalloc(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("regalloc");
  module.notes.push_back(PrepareNote{
      .phase = "regalloc",
      .message = "regalloc skeleton is wired; no physical register assignment is performed yet",
  });
}

}  // namespace c4c::backend::prepare
