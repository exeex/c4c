#include "prepare.hpp"

#include "legalize.hpp"
#include "liveness.hpp"
#include "regalloc.hpp"
#include "stack_layout.hpp"

namespace c4c::backend::prepare {

PreparedLirModule prepare_lir_module_with_options(
    const c4c::codegen::lir::LirModule& module,
    Target target,
    const PrepareOptions& options) {
  PreparedLirModule prepared{
      .module = module,
      .target = target,
      .completed_phases = {},
      .notes = {},
  };

  if (options.run_legalize) {
    run_legalize(prepared, options);
  }
  if (options.run_stack_layout) {
    run_stack_layout(prepared, options);
  }
  if (options.run_liveness) {
    run_liveness(prepared, options);
  }
  if (options.run_regalloc) {
    run_regalloc(prepared, options);
  }
  return prepared;
}

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options) {
  PreparedBirModule prepared{
      .module = module,
      .target = target,
      .completed_phases = {},
      .notes = {},
  };

  if (options.run_legalize) {
    run_legalize(prepared, options);
  }
  return prepared;
}

}  // namespace c4c::backend::prepare
