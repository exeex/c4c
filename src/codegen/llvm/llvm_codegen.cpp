#include "llvm_codegen.hpp"
#include "hir_emitter.hpp"

namespace c4c::codegen::llvm_backend {

std::string emit_module_native(const Module& mod) {
  HirEmitter emitter(mod);
  return emitter.emit();
}

}  // namespace tinyc2ll::codegen::llvm_backend
