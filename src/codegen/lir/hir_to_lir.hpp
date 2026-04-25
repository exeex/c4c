#pragma once

// Public HIR-to-LIR lowering entry point.
//
// External callers should include this header when converting HIR to LIR. The
// nested `hir_to_lir/` headers are private implementation indexes for lowering
// internals such as statement, expression, and call emission.

#include "ir.hpp"

namespace c4c::hir {
struct Module;
}

namespace c4c::codegen::lir {

struct LowerOptions {
  bool preserve_semantic_va_ops = false;
};

/// Lower a HIR module to a LIR module.
LirModule lower(const c4c::hir::Module& hir_mod, const LowerOptions& options = {});

}  // namespace c4c::codegen::lir
