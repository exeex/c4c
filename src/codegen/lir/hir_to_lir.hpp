#pragma once

// ── HIR-to-LIR lowering ─────────────────────────────────────────────────────
//
// Transforms a HIR Module into a LIR Module.
// This is a behavior-preserving extraction of the lowering logic currently
// embedded in HirEmitter.  Stage 0 provides only the interface skeleton;
// actual lowering will be migrated in Stage 1+.

#include "ir.hpp"

#include <string>

namespace c4c::hir {
struct Module;
}

namespace c4c::codegen::lir {

/// Lower a HIR module to a LIR module.
/// Stage 0: stub that returns an empty LirModule.
/// Stage 1+: will contain the full lowering pipeline.
LirModule lower(const c4c::hir::Module& hir_mod);

}  // namespace c4c::codegen::lir
