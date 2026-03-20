# Codegen Refactor Todo

## Status

In progress. Phases 0–4 slice 1 complete.

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

### Phase 3: Move Module-Level Lowering — slice 3 ✓

- Moved module-level finalization from `HirEmitter::lower_items()` to `hir_to_lir::lower()`:
  - Intrinsic flags (va_start, va_end, va_copy, memcpy, stacksave, stackrestore, abs)
  - Extern call declarations (functions called but not defined in TU)
  - Specialization metadata (spec entries)
- Added post-lowering accessors on `HirEmitter` for accumulated state
- Moved `SpecEntry` struct to public section of `HirEmitter`
- `HirEmitter::lower_to_lir()` now delegates entirely to `lir::lower()`
- New `finalize_module()` static helper in `hir_to_lir.cpp` owns all module-level finalization
- Both legacy and lir paths produce identical output; compare tests pass

### Phase 4: Move Function Skeleton Lowering — slice 0 (terminator separation) ✓

- Added `LirRawTerminator` variant to `LirTerminator` in `ir.hpp`
- Terminators stored in `LirBlock::terminator` field (was mixed into `insts`)
- `emit_term()` sets `LirBlock::terminator` instead of pushing to `insts`
- `emit_instr()` and `emit_lir_op()` skip dead code after terminator placed
- `render_terminator()` in printer handles `LirRawTerminator`

### Phase 4: Move Function Skeleton Lowering — slice 1 (iteration ownership) ✓

- Replaced `HirEmitter::lower_items()` with finer-grained API:
  - `adopt_module()` / `release_module()` — hand module in/out
  - `lower_globals(indices)` — lower globals only
  - `lower_single_function(fn)` — lower one function
- `hir_to_lir::lower()` now drives per-function iteration directly
- HirEmitter retains per-function body lowering (emit_function)
- Both legacy and lir paths produce identical output; all tests pass

### Test results

- Full suite: **2075/2075 passed** (no regressions)


## Immediate Next Work

The next slices, in priority order:

1. Phase 4 slice 2: Extract function signature building to hir_to_lir
2. Phase 4 slice 3: Move block ordering and fallthrough terminator to hir_to_lir
3. Phase 5: Move expression/statement lowering by risk slice


## Blockers

None.


## Definition Of Done

This todo is complete when:

- `hir_to_lir.cpp` is the real lowering owner for the new path
- `hir_emitter.cpp` is only the legacy path
- new and old codegen paths can be switched by flag
- compare mode can emit and diff two `.ll` files
- the new path is stable enough to serve as the base for a future LLVM API emitter
