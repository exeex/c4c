#include "liveness.hpp"

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
