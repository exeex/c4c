#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "../llvm/hir_emitter.hpp"

namespace c4c::codegen::lir {

LirModule lower(const c4c::hir::Module& hir_mod) {
  // Delegate to HirEmitter's structured lowering.
  // This produces an identical LirModule to what the legacy path builds,
  // but returns the structured representation instead of a printed string.
  c4c::codegen::llvm_backend::HirEmitter emitter(hir_mod);
  return emitter.lower_to_lir();
}

}  // namespace c4c::codegen::lir
