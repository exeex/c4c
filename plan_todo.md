# Plan Execution State

## Baseline
- 1829/1829 tests pass (2026-03-16)

## Active Plan: HIR Inline Expansion (inline_plan.md)

### Completed
- [x] Phase 0: Attribute and Policy Plumbing
  - Added `is_noinline` and `is_always_inline` to TypeSpec (ast.hpp)
  - `parse_attributes()` now captures `noinline`/`__noinline__` and `always_inline`/`__always_inline__`
  - Replaced `skip_attributes()` with `parse_attributes()` at function-definition sites in declarations.cpp
  - Propagated to HIR `FnAttr.no_inline` and `FnAttr.always_inline` in ast_to_hir.cpp
  - LLVM IR emission emits `noinline`/`alwaysinline` function attributes in hir_emitter.cpp
  - Added `inline_attrs.c` test case validating attribute parsing and IR emission
  - Precedence: noinline and always_inline stored independently; noinline wins at inline-expansion time (Phase 1+)
- [x] Phase 1: Call-site discovery and eligibility
  - Created `inline_expand.hpp` / `inline_expand.cpp` with Phase 1 APIs
  - `resolve_direct_callee()`: resolves CallExpr callee DeclRef → Function* via fn_index
  - `check_inline_eligibility()`: checks body present, not variadic, not self-recursive, not noinline, is always_inline
  - `discover_inline_candidates()`: scans all functions/blocks for CallExpr in ExprStmt, ReturnStmt, LocalDecl init
  - `run_inline_expansion()`: stub entry point wired into pipeline (c4cll.cpp, between HIR and LLVM emission)
  - Added `inline_call_discovery.c` test case with always_inline, noinline, and nested call patterns
  - Current policy: only `always_inline` callees are eligible; default/noinline rejected
- [x] Phase 2: ID remapping infrastructure
  - `InlineCloneContext` struct with local_map, block_map, expr_map, debug_prefix
  - `remap_local()`, `remap_block()`, `remap_expr()` — allocate fresh IDs on first encounter
  - `lookup_local()`, `lookup_block()`, `lookup_expr()` — query existing mappings
  - `clone_expr()` — deep-clones expression tree, remapping all sub-expr/local/block references
  - `clone_stmt()` — clones any statement variant, remapping locals, exprs, and block targets
  - `clone_block()` — clones all statements in a block with fresh BlockId
  - `preallocate_block_ids()` — pre-maps all callee blocks so forward references resolve during cloning
  - Handles all 16 ExprPayload variants and all 16 StmtPayload variants

### Next
- [ ] Phase 3: Argument capture and parameter binding
  - Synthesize one LocalDecl per callee parameter in caller
  - Emit initialization from actual arguments
  - Rewrite DeclRef.param_index in cloned body to those synthetic locals

### Previous Plan (Consteval) — Complete
- [x] Phase 1–4 of consteval plan fully implemented

## Blockers
- None
