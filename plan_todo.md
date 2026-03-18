# Plan Execution State

## Baseline
- 1897/1897 tests pass (2026-03-18)

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
  - Added `inline_cfg_splice.c` test: add1, add, void callee, side-effect-once args, chained inlines
- [x] Phase 5: General return rewriting and multi-block bodies
  - Removed single-block and single-return restrictions from expand_inline_site
  - All ReturnStmt in cloned callee blocks rewritten to assign + goto continuation
  - After rewriting a return, remaining stmts in that block trimmed as dead code
  - Supports if/else branches, while loops, for loops, and multi-return callees
  - Fixed iterator invalidation bug: clone_expr copies src Expr by value before recursive cloning (expr_pool push_back may reallocate)
  - Fixed iterator invalidation bug: expand_inline_site copies CallExpr by value before make_local_ref/make_assign (which push to expr_pool)
  - Added ArrayParam rejection in eligibility check: callees with array-typed parameters skipped (codegen GEP limitation)
  - Added `inline_multi_block.c` test: abs_val, clamp, sum_to_n, classify, swap_if_greater, factorial, chained calls
  - 1896/1896 tests pass (0 regressions, +1 new test)

- [x] Phase 6: Call replacement in expression contexts
  - Added `normalize_inline_calls()` pre-pass: hoists nested inline-eligible calls from expression trees into temp LocalDecls
  - `hoist_nested_calls()`: recursive walk of expression tree; non-root inline-eligible CallExpr → hoisted to temp
  - `is_hoistable_inline_call()`: checks inline eligibility + non-void return for hoisting
  - Handles BinaryExpr, UnaryExpr, CastExpr, TernaryExpr, AssignExpr, IndexExpr, MemberExpr, CallExpr args
  - Careful iterator-invalidation handling: payload copied by value before recursion, written back after
  - Normalization runs once before expansion loop in `run_inline_expansion()`
  - Added `inline_expr_ctx.c` test: 11 cases covering binary ops, casts, ternary, nested calls, unary
  - 1897/1897 tests pass (0 regressions, +1 new test)

### Next
- [ ] Phase 7: Diagnostics and unsupported cases
  - Produce clear reasons for refusal (recursive, indirect, variadic, unsupported form)
  - Ensure noinline always suppresses expansion
  - Decide permissive vs strict mode

### Previous Plan (Consteval) — Complete
- [x] Phase 1–4 of consteval plan fully implemented

## Blockers
- ArrayParam: callees with array-typed parameters cannot be inlined yet (codegen produces wrong GEP for synthetic local vs parameter)
