#pragma once

// ── HIR-to-LIR lowering ─────────────────────────────────────────────────────
//
// Transforms a HIR Module into a LIR Module.
// Module-level orchestration (dedup, type decls, target setup) lives here.
// Per-statement lowering is still delegated to HirEmitter.

#include "ir.hpp"
#include "../shared/fn_lowering_ctx.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace c4c::hir {
struct Module;
struct Function;
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

// ── Per-function skeleton helpers (usable by both lir path and legacy) ────────

/// Find parameter indices that are modified (assigned to, incremented, or
/// address-taken) anywhere in the function body.
std::unordered_set<uint32_t> find_modified_params(
    const c4c::hir::Module& mod, const c4c::hir::Function& fn);

/// Returns true if the function has any VLA (variable-length array) locals.
bool fn_has_vla_locals(const c4c::hir::Function& fn);

/// Hoist alloca instructions for all locals and spilled parameters.
/// Populates ctx.alloca_insts, ctx.local_slots, ctx.local_types, ctx.local_is_vla.
void hoist_allocas(c4c::codegen::FnCtx& ctx, const c4c::hir::Module& mod,
                   const c4c::hir::Function& fn);

/// Initialize a FnCtx for the given function: set up param_slots, fn_ptr_sigs,
/// create entry block, hoist allocas, and optionally emit VLA stack save.
/// Returns a fully-initialized FnCtx ready for statement emission.
c4c::codegen::FnCtx init_fn_ctx(const c4c::hir::Module& mod,
                                 const c4c::hir::Function& fn);

}  // namespace c4c::codegen::lir

namespace c4c::hir { struct Block; struct BlockId; }

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

/// Map a HIR BlockId to its LLVM IR label string.
/// Ownership of block naming belongs to hir_to_lir, not HirEmitter.
std::string block_lbl(c4c::hir::BlockId id);

/// Create a new LIR block with the given label and make it current in ctx.
/// Ownership of block creation belongs to hir_to_lir, not HirEmitter.
void emit_lbl(c4c::codegen::FnCtx& ctx, const std::string& lbl);

}  // namespace c4c::codegen::lir
