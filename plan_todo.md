# Codegen Refactor Todo

## Status

In progress. Phase 5 slice 0 complete.

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

### Phase 4: Move Function Skeleton Lowering — slice 2 (signature building) ✓

- Extracted function signature building into `lir::build_fn_signature()` free function in `hir_to_lir.cpp`
- Moved `llvm_global_sym()` and `llvm_visibility()` from anonymous namespace in `hir_emitter.cpp` to shared `hir_to_llvm_helpers.hpp`
- `hir_to_lir::lower()` now builds signature text and passes it to `lower_single_function(fn, sig)`
- `emit_function()` accepts optional pre-built signature; legacy path builds it inline
- Spec entry collection moved to after signature selection (works for both paths)
- param_slots populated independently of signature source
- Both legacy and lir paths produce identical output; all compare tests pass

### Phase 4: Move Function Skeleton Lowering — slice 3 (block ordering + fallthrough) ✓

- Extracted `build_block_order(fn)` free function in `hir_to_lir.cpp`:
  computes HIR block iteration order (entry first, rest in original order)
- Extracted `inject_fallthrough_returns(lir_fn, fn)` post-pass in `hir_to_lir.cpp`:
  walks LirBlocks after lowering; replaces `LirUnreachable` terminators with default returns
- `hir_to_lir::lower()` now passes block order to `lower_single_function(fn, sig, block_order)`
- `emit_function()` uses externally-provided block order when available; computes internally for legacy
- Fallthrough return generation removed from `emit_function()`; now a post-pass in `hir_to_lir::lower()`
- Both legacy and lir paths produce identical output; all compare tests pass

### Phase 4: Move Function Skeleton Lowering — slice 4 (FnCtx + alloca hoisting) ✓

- Moved `FnCtx` and `BlockMeta` to shared header `src/codegen/shared/fn_lowering_ctx.hpp`
- Extracted `find_modified_params()`, `fn_has_vla_locals()`, `hoist_allocas()` as free functions in `hir_to_lir.cpp`
- Created `init_fn_ctx()` in `hir_to_lir.cpp`: initializes FnCtx with param_slots, fn_ptr_sigs, entry block, and hoisted allocas
- Added `HirEmitter::emit_function_body(ctx, sig, block_order)` that accepts pre-initialized FnCtx
- `hir_to_lir::lower()` now builds FnCtx externally via `init_fn_ctx()` for function definitions
- HirEmitter retains `emit_function()` for legacy path (still creates its own FnCtx internally)
- Both legacy and lir paths produce identical output; all compare tests pass

### Phase 5: Move Expression/Statement Lowering — slice 0 (remaining skeleton) ✓

- Moved VLA stack save from `emit_function_body()` to `init_fn_ctx()` in `hir_to_lir.cpp`
- Moved spec entry collection from `emit_function_body()` to `lower()` loop
- Moved LirFunction construction from `emit_function_body()` to `lower()` loop
- Moved fallthrough return injection to per-function post-pass (was global post-pass)
- Added `HirEmitter::push_lir_function()` for hir_to_lir to push pre-built functions
- `emit_function_body()` now only does block iteration + statement emission
- `lower()` tracks `any_vla` flag and sets `module.need_stacksave` directly
- Both legacy and lir paths produce identical output; all compare tests pass

### Test results

- Full suite: **2075/2075 passed** (no regressions)


## Immediate Next Work

The next slices, in priority order:

1. Phase 5 slice 1: simple expressions (literals, decl refs, casts, basic arithmetic/compare)


## Blockers

None.


## Definition Of Done

This todo is complete when:

- `hir_to_lir.cpp` is the real lowering owner for the new path
- `hir_emitter.cpp` is only the legacy path
- new and old codegen paths can be switched by flag
- compare mode can emit and diff two `.ll` files
- the new path is stable enough to serve as the base for a future LLVM API emitter
