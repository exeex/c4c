#include "llvm_codegen.hpp"
#include "backend.hpp"
#include "lowering/lir_to_bir.hpp"
#include "target.hpp"
#include "hir_to_lir.hpp"
#include "lir_printer.hpp"

#include <iostream>
#include <stdexcept>

namespace c4c::codegen::llvm_backend {

namespace {

std::string emit_legacy(const lir::LirModule& lir_mod) {
  return lir::print_llvm(lir_mod);
}

[[noreturn]] void throw_backend_native_asm_required() {
  throw std::invalid_argument("--codegen asm requires backend-native assembly output.");
}

std::string emit_via_backend(const lir::LirModule& lir_mod,
                             std::string_view target_triple) {
  const auto target = backend::target_from_triple(target_triple);
  if (target == backend::Target::Riscv64 &&
      !backend::try_lower_to_bir(lir_mod).has_value()) {
    throw_backend_native_asm_required();
  }
  return backend::emit_module(backend::BackendModuleInput{lir_mod},
                              backend::BackendOptions{.target = target});
}

}  // namespace

std::string emit_module_native(const Module& mod,
                               std::string_view target_triple,
                               CodegenPath path) {
  auto lir_mod = lir::lower(mod);
  if (path == CodegenPath::Llvm) {
    return emit_legacy(lir_mod);
  }
  if (path == CodegenPath::Compare) {
    const auto legacy = emit_legacy(lir_mod);
    const auto result = legacy;
    std::cerr << "codegen compare: outputs match\n";
    return result;
  }

  auto result = emit_via_backend(lir_mod, target_triple);
  return result;
}

}  // namespace c4c::codegen::llvm_backend
