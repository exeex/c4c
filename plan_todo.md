# Codegen Refactor Todo

## Status

In progress. Phases 0–3 slice 2 complete.

## Completed

### Phase 0: Re-baseline The Plan ✓

- plan and todo accurately reflect that lowering split was not complete

### Phase 1: Add New/Old Path Selection ✓

- `--codegen=legacy|lir|compare` CLI flag wired in `c4cll.cpp`
- `llvm_codegen.cpp` routes to `emit_legacy()` or `emit_lir()` based on flag

### Phase 2: Implement Compare Mode ✓

- compare mode runs both paths, diffs output line-by-line
- reports match/mismatch to stderr, returns legacy output for test compatibility

### Phase 3: Move Module-Level Lowering — slice 1 ✓

- Added `HirEmitter::lower_to_lir()` public method that returns `LirModule`
- Refactored `HirEmitter::emit()` to call `lower_to_lir()` + `lir::print_llvm()`
- `hir_to_lir::lower()` now delegates to `HirEmitter::lower_to_lir()`
- LIR path (`--codegen=lir`) produces identical output to legacy path
- Compare mode (`--codegen=compare`) confirms outputs match
- 3 compare-mode smoke tests added: `smoke_scalar`, `smoke_struct`, `smoke_switch`

### Phase 3: Move Module-Level Lowering — slice 2 ✓

- Extracted module-level orchestration from `HirEmitter` into standalone free functions:
  - `lir::dedup_globals()` — global dedup logic (tentative def resolution)
  - `lir::dedup_functions()` — function dedup logic (prefer definitions over declarations)
  - `lir::build_type_decls()` — struct/union type declaration generation (was `emit_preamble()`)
- `hir_to_lir::lower()` now owns module construction: target triple, data_layout, type_decls, dedup
- Added `HirEmitter::lower_items(module, global_indices, fn_indices)` for per-item lowering
- `HirEmitter::lower_to_lir()` delegates to same shared helpers for consistency
- Removed dead `emit_preamble()` from HirEmitter
- Both legacy and lir paths produce identical output; compare tests pass

### Test results

- Full suite: **2073/2073 passed** (no regressions)


## Immediate Next Work

The next slices, in priority order:

1. Phase 3 slice 3: Move string pool, extern decl tracking, and intrinsic flags to LIR lowering layer
2. Phase 4: Move function skeleton lowering (FnCtx, blocks, terminators)
3. Phase 5: Move expression/statement lowering by risk slice
4. Phase 7: Expand compare-mode smoke suite (varargs, bitfields, indirectbr, etc.)


## Blockers

None.


## Definition Of Done

This todo is complete when:

- `hir_to_lir.cpp` is the real lowering owner for the new path
- `hir_emitter.cpp` is only the legacy path
- new and old codegen paths can be switched by flag
- compare mode can emit and diff two `.ll` files
- the new path is stable enough to serve as the base for a future LLVM API emitter
