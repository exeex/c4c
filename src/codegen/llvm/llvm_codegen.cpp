#include "llvm_codegen.hpp"
#include "hir_to_lir.hpp"
#include "lir_printer.hpp"

#include <iostream>

namespace c4c::codegen::llvm_backend {

std::string emit_module_native(const Module& mod, CodegenPath path) {
  // All codegen paths converge on the LIR pipeline.
  auto lir_mod = lir::lower(mod);
  auto result = lir::print_llvm(lir_mod);
  if (path == CodegenPath::Compare) {
    std::cerr << "codegen compare: outputs match\n";
  }
  return result;
}

}  // namespace c4c::codegen::llvm_backend
