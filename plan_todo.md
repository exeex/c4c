# HIR → LIR Split — Execution State

## Current Step: Step 2 (Remove One Semantic Dependency At A Time)

## Step 1 Audit: Legacy Dependencies in hir_to_lir.cpp

### Semantic lowering dependencies (real problems):
1. **`emitter.lower_globals(global_indices)`** — entire global lowering delegated to HirEmitter
2. **`emitter.emit_stmt(ctx, stmt)`** — entire statement/expression lowering delegated to HirEmitter
3. **`emitter.lower_single_function(fn, sig)`** — declaration fn lowering delegated to HirEmitter

### Module ownership / orchestration:
4. ~~`emitter.adopt_module()` / `emitter.release_module()` / `emitter.push_lir_function()`~~ — **DONE** (replaced with reference pattern)

### Type/helper reuse (printer-level formatting used during lowering):
5. `llvm_backend::detail::*` helpers used throughout: `llvm_ty()`, `llvm_ret_ty()`, `llvm_alloca_ty()`, `sanitize_llvm_ident()`, `llvm_struct_type_str()`, `llvm_field_ty()`, `llvm_global_sym()`, `llvm_visibility()`, `set_active_target_triple()`, `fp_literal()`, `is_float_base()`, `is_complex_base()`, `quote_llvm_ident()`, `llvm_default_datalayout()`, `llvm_va_list_is_pointer_object()`

### Summary:
- Items 1-3 are the remaining semantic dependencies; extracting them requires significant work
- Item 4 (adopt/release) is eliminated — module is now ref-based
- Item 5 is acceptable for now (shared helpers, not semantic ownership)

## Completed Items
- [x] Step 1: Audit current legacy dependencies
- [x] Step 2a: Extract `emit_lbl()` and `block_lbl()` into hir_to_lir (items 3-4 from original audit)
- [x] Step 2b: Replace adopt/release module pattern with reference-based `set_module()`

## Active Slice
- (none — ready for next iteration)

## Next Intended Slice
- Extract declaration lowering from HirEmitter (`lower_single_function` for extern decls) into hir_to_lir
- Or: extract `lower_globals` into hir_to_lir (requires moving `emit_global` + const init logic)

## Blockers
- None
