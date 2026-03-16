# Plan Execution State

## Baseline
- Test suite: **1800/1800** (2026-03-16, after Cleanup item 1)

## Completed
- Phase A: Lock Current Behavior With Tests — existing tests cover fn-ptr return cases; XFAIL guard exists for nested raw syntax
- Phase B: Fill `ResolvedTypeTable` For Locals — `record_local_decl_types()` recursively walks function bodies and records canonical types for all NK_DECL nodes
- Phase C slice 1: Typedef fn_ptr param propagation + struct member fn_ptr sig resolution
- Phase C slice 2: CastExpr, IndexExpr, CallExpr fn_ptr sig resolution
- Phase C slice 3: ret_fn_ptr_sig zero-param fix + CallExpr cleanup
  - Bugfix: `lower_function` ret_fn_ptr_sig condition widened from `is_fn_ptr && n_fn_ptr_params > 0` to `is_fn_ptr`
  - Codegen: reorganized CallExpr return type resolution
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators
  - Added `ret_fn_ptr_params`, `n_ret_fn_ptr_params`, `ret_fn_ptr_variadic` fields to Node
  - Parser: `parse_declarator` captures both levels of fn_ptr params for nested declarators
  - Parser: `parse_global_decl_or_function` handles `(* (*name(params))(inner_params))(outer_params)` pattern
  - HIR: `fn_ptr_sig_from_decl_node` legacy path sets `is_fn_ptr=true, ptr_level=1` on return type when `ret_fn_ptr_params` present
  - Test: `0009_nested_fn_returning_fn_ptr.c` XFAIL flipped to pass

## Active Item
- **Phase D: Delete Legacy Callable Recovery** — now unblocked by parser gap fix

## Next Slice
- Remove `local_fn_ret_types` map — now that nested fn_ptr return types are correctly represented in fn_ptr_sig, the workaround should be redundant
- Remove TypeSpec-based call-result peeling in CallExpr return type resolution
- Verify by running test suite after each removal

## Blockers
- None known

## Deferred
- (none)
