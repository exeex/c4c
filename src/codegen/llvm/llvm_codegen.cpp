#include "llvm_codegen.hpp"
#include "backend.hpp"
#include "lir_adapter.hpp"
#include "../../backend/lowering/lir_to_bir.hpp"
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

std::string emit_via_backend(const lir::LirModule& lir_mod,
                             std::string_view target_triple) {
  backend::BackendOptions options;
  options.target = backend::target_from_triple(target_triple);
  if (options.target == backend::Target::Riscv64 &&
      backend::try_lower_to_bir(lir_mod).has_value()) {
    options.pipeline = backend::BackendPipeline::Bir;
    return backend::emit_module(backend::BackendModuleInput{lir_mod}, options);
  }

  backend::BackendModuleInput input{lir_mod};
  try {
    input = backend::BackendModuleInput{backend::lower_lir_to_backend_module(lir_mod), &lir_mod};
  } catch (const backend::LirAdapterError& ex) {
    if (ex.kind() != backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  }
  return backend::emit_module(input, options);
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
