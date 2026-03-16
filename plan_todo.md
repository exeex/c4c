# Plan Execution State

## Baseline
- Test suite: **1800/1800** (2026-03-16, after Phase C slice 3)

## Completed
- Phase A: Lock Current Behavior With Tests — existing tests cover fn-ptr return cases; XFAIL guard exists for nested raw syntax
- Phase B: Fill `ResolvedTypeTable` For Locals — `record_local_decl_types()` recursively walks function bodies and records canonical types for all NK_DECL nodes
- Phase C slice 1: Typedef fn_ptr param propagation + struct member fn_ptr sig resolution
  - Parser: capture fn_ptr_params when storing fn_ptr typedefs in `typedef_fn_ptr_info_`
  - Parser: propagate typedef fn_ptr params to struct field, local decl, global decl, and param nodes via `last_resolved_typedef_`
  - HIR: `HirStructField` now has `optional<FnPtrSig> fn_ptr_sig` for callable fields
  - Lowering: `lower_struct_def()` canonicalizes field types directly to build fn_ptr sigs
  - Codegen: `resolve_callee_fn_ptr_sig()` handles MemberExpr (struct field fn_ptr) and TernaryExpr
  - Test: `ok_expr_canonical_types.c` covers struct member, local, param, and ternary fn_ptr calls
- Phase C slice 2: CastExpr, IndexExpr, CallExpr fn_ptr sig resolution
  - Parser: propagate fn_ptr params from typedef onto NK_CAST nodes (expressions.cpp)
  - Parser: propagate return-type fn_ptr params onto NK_FUNCTION nodes (declarations.cpp)
  - HIR: `CastExpr` now has `optional<FnPtrSig> fn_ptr_sig` for casts to callable types
  - HIR: `Function` now has `optional<FnPtrSig> ret_fn_ptr_sig` for functions returning fn_ptrs
  - Lowering: NK_CAST populates fn_ptr_sig from fn_ptr_sig_from_decl_node
  - Lowering: lower_function builds ret_fn_ptr_sig from fn_node's fn_ptr_params
  - Codegen: resolve_callee_fn_ptr_sig handles CastExpr, IndexExpr (recurse on base), and CallExpr (lookup ret_fn_ptr_sig)
  - Bugfix: canonicalize_fn_ptr_type + fn_ptr_sig_from_decl_node strip declarator array dims from return type
  - Test: `ok_expr_canonical_cast_index.c` covers cast, index, and call-returning-fn_ptr patterns
- Phase C slice 3: ret_fn_ptr_sig zero-param fix + CallExpr resolution cleanup
  - Bugfix: `lower_function` ret_fn_ptr_sig condition widened from `is_fn_ptr && n_fn_ptr_params > 0` to `is_fn_ptr` — fixes functions returning zero-param fn_ptrs (e.g. `struct S * (*)()`)
  - Codegen: reorganized CallExpr return type resolution — canonical fn_ptr_sig now checked before fn_index direct-call lookup; removed redundant DeclRef-specific TypeSpec peeling (covered by general callee_ts path)
  - Codegen: cleaned up stale phase comments and TODO markers

## Active Item
- **Phase C: Add Expression Canonical Types** (remaining slices)

## Next Slice
- Phase D: Legacy fallback removal — blocked by parser gap for nested fn-returning-fn-ptr declarators
  - Attempted removal of `local_fn_ret_types` map revealed that canonical type system doesn't correctly represent nested fn_ptr return types (e.g. `int (* (*p)(int, int))(int, int)`)
  - The `local_fn_ret_types` workaround must remain until the parser gap is fixed
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators — prerequisite for Phase D

## Blockers
- Phase D blocked by parser gap (Cleanup item 1): canonical types inherit the parser's inability to fully represent nested fn-ptr return types

## Deferred
- Phase D: Legacy fallback removal (needs parser gap fix first)
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators
