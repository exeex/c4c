#include "liveness.hpp"

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/liveness.rs for the intended
// liveness/dataflow analysis shape.

namespace c4c::backend::prepare {

void run_liveness(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("liveness");
  module.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message = "liveness skeleton is wired; no live-interval computation is performed yet",
  });
}

}  // namespace c4c::backend::prepare
