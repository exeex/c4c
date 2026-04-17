#include "x86_codegen.hpp"
#include "peephole/peephole.hpp"

#include "../../backend.hpp"
#include "../../../bir/lir_to_bir.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

const c4c::TargetProfile& resolve_direct_bir_target_profile(
    const c4c::backend::bir::Module& module,
    c4c::TargetProfile& storage) {
  storage = c4c::target_profile_from_triple(
      module.target_triple.empty() ? c4c::default_host_target_triple() : module.target_triple);
  if (storage.arch != c4c::TargetArch::X86_64 && storage.arch != c4c::TargetArch::I686) {
    throw std::invalid_argument(
        "x86 backend emitter requires an x86 target triple for canonical prepared-module handoff");
  }
  return storage;
}

[[noreturn]] void throw_x86_rewrite_in_progress() {
  throw std::invalid_argument(
      "x86 backend emitter rewrite in progress: move ownership into the translated sibling codegen translation units instead of adding emit.cpp-local matchers");
}

}  // namespace

std::string emit_module(const c4c::backend::bir::Module& module) {
  c4c::TargetProfile target_profile;
  const auto prepared = c4c::backend::prepare::prepare_semantic_bir_module_with_options(
      module, resolve_direct_bir_target_profile(module, target_profile));
  return emit_prepared_module(prepared);
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = c4c::backend::try_lower_to_bir(module); lowered.has_value()) {
    return emit_module(*lowered);
  }
  throw_x86_rewrite_in_progress();
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(assembler::AssembleRequest{
      .asm_text = emit_module(module),
      .output_path = output_path,
  });
}

}  // namespace c4c::backend::x86
