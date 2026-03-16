# Plan Execution State

## Baseline
- Test suite: **1799/1799** (2026-03-16, after Phase C slice 1)

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

## Active Item
- **Phase C: Add Expression Canonical Types** (remaining slices)

## Next Slice
- Handle CastExpr in resolve_callee_fn_ptr_sig() (requires parser to capture fn_ptr_params on NK_CAST nodes)
- Handle IndexExpr (array of fn_ptrs)
- Handle CallExpr (return value is fn_ptr)
- Remove legacy TypeSpec-based call-result peeling once expression types are fully covered (Phase D overlap)

## Blockers
- NK_CAST fn_ptr param propagation requires extending parse_type_name() to output fn_ptr info

## Deferred
- Phase D: Legacy fallback removal
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators
