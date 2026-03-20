#pragma once

// ── HIR-to-LIR lowering ─────────────────────────────────────────────────────
//
// Transforms a HIR Module into a LIR Module.
// Module-level orchestration (dedup, type decls, target setup) lives here.
// Per-item lowering (globals, functions) is still delegated to HirEmitter
// until later phases migrate it.

#include "ir.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace c4c::hir {
struct Module;
}

namespace c4c::codegen::lir {

/// Lower a HIR module to a LIR module.
LirModule lower(const c4c::hir::Module& hir_mod);

// ── Module-level orchestration helpers (usable by both lir path and legacy) ──

/// Deduplicate globals: prefer entries with explicit initializers; among
/// equals, prefer later entries (last-wins for extern/tentative semantics).
/// Returns indices into mod.globals of the "best" entry per name, in
/// original order.
std::vector<size_t> dedup_globals(const c4c::hir::Module& mod);

/// Deduplicate functions: prefer definitions (non-empty blocks) over
/// declarations.  Skips non-materialized functions.
/// Returns indices into mod.functions of the "best" entry per name, in
/// original order.
std::vector<size_t> dedup_functions(const c4c::hir::Module& mod);

/// Build LLVM struct/union type declaration strings from the HIR module's
/// struct definitions.
std::vector<std::string> build_type_decls(const c4c::hir::Module& mod);

}  // namespace c4c::codegen::lir

namespace c4c::hir { struct Function; struct Block; }

namespace c4c::codegen::lir {

/// Build the LLVM IR signature text for a function (define/declare line).
/// This includes template-origin comments, linkage, visibility, return type,
/// parameter list, variadic marker, and function attributes.
/// Ownership of signature construction belongs to hir_to_lir, not HirEmitter.
std::string build_fn_signature(const c4c::hir::Function& fn);

/// Compute the HIR block iteration order for a function: entry block first,
/// then remaining blocks in their original order.
/// Ownership of block ordering belongs to hir_to_lir, not HirEmitter.
std::vector<const c4c::hir::Block*>
build_block_order(const c4c::hir::Function& fn);

}  // namespace c4c::codegen::lir
