#include "prealloc.hpp"

#include "legalize.hpp"
#include "liveness.hpp"
#include "regalloc.hpp"
#include "stack_layout.hpp"

namespace c4c::backend::prepare {

BirPreAlloc::BirPreAlloc(const c4c::backend::bir::Module& module,
                         Target target,
                         const PrepareOptions& options)
    : module_(module),
      target_(target),
      options_(options),
      prepared_{
          .module = module,
          .target = target,
          .route = PrepareRoute::SemanticBirShared,
          .completed_phases = {},
          .notes = {},
      } {}

void BirPreAlloc::note(std::string_view message) {
  prepared_.notes.push_back(PrepareNote{
      .phase = "prepare",
      .message = std::string(message),
  });
}

PreparedBirModule BirPreAlloc::run() {
  note("prepare owns the shared semantic-BIR to prepared-BIR route before target codegen");

  if (options_.run_legalize) {
    run_legalize(prepared_, options_);
  }
  if (options_.run_stack_layout) {
    run_stack_layout(prepared_, options_);
  }
  if (options_.run_liveness) {
    run_liveness(prepared_, options_);
  }
  if (options_.run_regalloc) {
    run_regalloc(prepared_, options_);
  }
  return std::move(prepared_);
}

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options) {
  return BirPreAlloc(module, target, options).run();
}

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options) {
  return prepare_semantic_bir_module_with_options(module, target, options);
}

}  // namespace c4c::backend::prepare
