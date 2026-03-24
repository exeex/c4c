# HIR → LIR Split — Execution State

## Current Step: Step 2 (Remove One Semantic Dependency At A Time)

## Step 1 Audit: Legacy Dependencies in hir_to_lir.cpp

### Semantic lowering dependencies (real problems):
1. **`emitter.lower_globals(global_indices)`** — entire global lowering delegated to HirEmitter
2. **`emitter.emit_stmt(ctx, stmt)`** — entire statement/expression lowering delegated to HirEmitter
3. **`emitter.emit_lbl(ctx, lbl)`** — block label creation delegated to HirEmitter ← TRIVIAL, extract first
4. **`emitter.block_lbl(id)`** — block label naming delegated to HirEmitter ← TRIVIAL, extract first

### Module ownership / orchestration:
5. `emitter.adopt_module()` / `emitter.release_module()` / `emitter.push_lir_function()` — module pass-through pattern
6. `finalize_module()` reads `emitter.needs_*()`, `emitter.extern_call_decls()`, `emitter.spec_entries()`

### Type/helper reuse (printer-level formatting used during lowering):
7. `llvm_backend::detail::*` helpers used throughout: `llvm_ty()`, `llvm_ret_ty()`, `llvm_alloca_ty()`, `sanitize_llvm_ident()`, `llvm_struct_type_str()`, `llvm_field_ty()`, `llvm_global_sym()`, `llvm_visibility()`, `set_active_target_triple()`, `fp_literal()`, `is_float_base()`, `is_complex_base()`, `quote_llvm_ident()`, `llvm_default_datalayout()`, `llvm_va_list_is_pointer_object()`

### Summary:
- Items 1-2 are the big semantic dependencies; extracting them requires significant work
- Items 3-4 are trivial helpers that can be moved immediately
- Items 5-6 are orchestration overhead from the adopt/release pattern
- Item 7 is acceptable for now (shared helpers, not semantic ownership)

## Completed Items
- [x] Step 1: Audit current legacy dependencies
- [x] Step 2a: Extract `emit_lbl()` and `block_lbl()` into hir_to_lir (items 3-4)

## Active Slice
- (none — ready for next iteration)

## Next Intended Slice
- Investigate extracting `lower_globals()` or reducing adopt/release pattern

## Blockers
- None
