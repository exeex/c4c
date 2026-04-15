#include "prealloc.hpp"

namespace c4c::backend::prepare {

void BirPreAlloc::note(std::string_view message) {
  prepared_.notes.push_back(PrepareNote{
      .phase = "prealloc",
      .message = std::string(message),
  });
}

PreparedBirModule BirPreAlloc::run() {
  note("prealloc owns the shared semantic-BIR to prealloc-BIR route before MIR lowering");
  run_legalize();
  run_stack_layout();
  run_liveness();
  run_regalloc();
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
