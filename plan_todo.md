# Plan Execution State

## Baseline
- 1831/1831 tests pass (2026-03-16)

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
- [x] Phase 3: Argument capture and parameter binding
  - `param_to_local` map added to InlineCloneContext
  - `bind_callee_params()`: synthesizes one LocalDecl per callee parameter, initialized with call argument
  - `clone_expr` DeclRef handling: rewrites `param_index` to synthetic local via `param_to_local` map
  - Added `inline_param_bind.c` test case: multi-param, void callee, side-effect-once, nested calls
- [x] Phase 4: Minimal CFG splice for single-return bodies
  - `expand_inline_site()`: splits caller block, clones callee body, rewrites returns, wires control flow
  - `run_inline_expansion()`: iterative loop discovering and expanding one candidate at a time
  - Handles ExprStmt, LocalDecl init, and ReturnStmt call contexts
  - Creates synthetic result local for non-void callees; assigns return value via AssignExpr
  - Rewrites ReturnStmt in cloned body to assign + GotoStmt to continuation block
  - Adds GotoStmt to continuation for blocks without explicit terminators (void callees with no return)
  - Handles `f(void)` parameter lists correctly (skip binding for single void param)
  - Phase 4 restrictions: single-return, single-block callees only; no va_arg in callee body
  - Added `inline_cfg_splice.c` test: add1, add, void callee, side-effect-once args, chained inlines
  - 1831/1831 tests pass (0 regressions, +1 new test)

### Next
- [ ] Phase 5: General return rewriting and multi-block bodies
  - Support multiple return sites (all rewrite to single continuation)
  - Support conditionals and loops in inlined bodies
  - Remove single-block restriction
  - Remap break/continue/switch targets within cloned region

### Previous Plan (Consteval) — Complete
- [x] Phase 1–4 of consteval plan fully implemented

## Blockers
- None
