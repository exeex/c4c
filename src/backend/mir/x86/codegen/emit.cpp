#include "x86_codegen.hpp"
#include "api/x86_codegen_api.hpp"
#include "peephole/peephole.hpp"

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

const c4c::TargetProfile& resolve_direct_lir_target_profile(
    const c4c::codegen::lir::LirModule& module,
    c4c::TargetProfile& storage) {
  storage = module.target_profile.arch != c4c::TargetArch::Unknown
                ? module.target_profile
                : c4c::target_profile_from_triple(c4c::default_host_target_triple());
  if (storage.arch != c4c::TargetArch::X86_64 && storage.arch != c4c::TargetArch::I686) {
    throw std::invalid_argument(
        "x86 backend emitter requires an x86 target profile for compatibility handoff through the canonical api seam");
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
  return c4c::backend::x86::api::emit_module(
      module, resolve_direct_bir_target_profile(module, target_profile));
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  c4c::TargetProfile target_profile;
  try {
    return c4c::backend::x86::api::emit_module(
        module, resolve_direct_lir_target_profile(module, target_profile));
  } catch (const std::invalid_argument& error) {
    if (std::string_view{error.what()}.find("lir_to_bir lowering") != std::string_view::npos) {
      throw_x86_rewrite_in_progress();
    }
    throw;
  }
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  c4c::TargetProfile target_profile;
  const auto assembled = c4c::backend::x86::api::assemble_module(
      module, resolve_direct_lir_target_profile(module, target_profile), output_path);
  return assembler::AssembleResult{
      .staged_text = std::move(assembled.staged_text),
      .output_path = std::move(assembled.output_path),
      .object_emitted = assembled.object_emitted,
      .error = std::move(assembled.error),
  };
}

}  // namespace c4c::backend::x86
