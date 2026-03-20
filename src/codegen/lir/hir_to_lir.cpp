#include "hir_to_lir.hpp"
#include "ir.hpp"

namespace c4c::codegen::lir {

LirModule lower(const c4c::hir::Module& /*hir_mod*/) {
  // Stage 0: return empty module.
  // Stage 1+ will populate this with actual lowered content.
  return LirModule{};
}

}  // namespace c4c::codegen::lir
