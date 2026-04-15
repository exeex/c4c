#include "prepare.hpp"

#include "legalize.hpp"
#include "liveness.hpp"
#include "regalloc.hpp"
#include "stack_layout.hpp"

namespace c4c::backend::prepare {

namespace {

void note_prepare_entry(PreparedBirModule& prepared, std::string_view message) {
  prepared.notes.push_back(PrepareNote{
      .phase = "prepare",
      .message = std::string(message),
  });
}

}  // namespace

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options) {
  PreparedBirModule prepared{
      .module = module,
      .target = target,
      .route = PrepareRoute::SemanticBirShared,
      .completed_phases = {},
      .notes = {},
  };
  note_prepare_entry(
      prepared,
      "prepare owns the shared semantic-BIR to prepared-BIR route before target codegen");

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
  return prepare_semantic_bir_module_with_options(module, target, options);
}

}  // namespace c4c::backend::prepare
