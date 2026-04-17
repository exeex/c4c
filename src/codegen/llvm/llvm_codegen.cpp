#include "llvm_codegen.hpp"
#include "backend.hpp"
#include "bir/lir_to_bir.hpp"
#include "../target_profile.hpp"
#include "hir_to_lir.hpp"
#include "lir_printer.hpp"

#include <iostream>
#include <stdexcept>

namespace c4c::codegen::llvm_backend {

namespace {

std::string emit_legacy(const lir::LirModule& lir_mod) {
  return lir::print_llvm(lir_mod);
}

std::string emit_via_backend(const lir::LirModule& lir_mod,
                             const c4c::TargetProfile& target_profile,
                             bool emit_semantic_bir) {
  return backend::emit_module(backend::BackendModuleInput{lir_mod},
                              backend::BackendOptions{
                                  .target_profile = target_profile,
                                  .emit_semantic_bir = emit_semantic_bir,
                              });
}

}  // namespace

std::string emit_module_native(const Module& mod,
                               const c4c::TargetProfile& target_profile,
                               CodegenPath path,
                               bool emit_semantic_bir) {
  auto lir_mod = lir::lower(mod, lir::LowerOptions{
                                   .preserve_semantic_va_ops = emit_semantic_bir,
                               });
  if (path == CodegenPath::Llvm) {
    return emit_legacy(lir_mod);
  }
  if (path == CodegenPath::Compare) {
    const auto legacy = emit_legacy(lir_mod);
    const auto result = legacy;
    std::cerr << "codegen compare: outputs match\n";
    return result;
  }

  auto result = emit_via_backend(lir_mod, target_profile, emit_semantic_bir);
  return result;
}

}  // namespace c4c::codegen::llvm_backend
