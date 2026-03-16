# Plan Execution State

## Baseline
- Test suite: **1800/1800** (2026-03-16, after Phase C slice 2)

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

## Active Item
- **Phase C: Add Expression Canonical Types** (remaining slices)

## Next Slice
- Remove legacy TypeSpec-based call-result peeling once expression types are fully covered (Phase D overlap)
- Handle remaining expression forms (NK_ADDR, CommaExpr) if needed by test cases

## Blockers
- None known

## Deferred
- Phase D: Legacy fallback removal
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators
