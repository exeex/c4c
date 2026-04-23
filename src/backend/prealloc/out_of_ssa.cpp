#include "prealloc.hpp"

namespace c4c::backend::prepare {

void out_of_ssa_module(bir::Module& module,
                       PreparedNameTables& names,
                       PreparedControlFlow* control_flow);

void BirPreAlloc::run_out_of_ssa() {
  prepared_.completed_phases.push_back("out_of_ssa");
  out_of_ssa_module(prepared_.module, prepared_.names, &prepared_.control_flow);
  prepared_.invariants.push_back(PreparedBirInvariant::NoPhiNodes);
  prepared_.notes.push_back(PrepareNote{
      .phase = "out_of_ssa",
      .message =
          "out_of_ssa removed phi nodes from live prepared BIR and published authoritative "
          "join-transfer plus parallel-copy obligations",
  });
}

}  // namespace c4c::backend::prepare
