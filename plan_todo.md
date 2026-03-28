# Deferred NTTP Expression Parser Gap Todo

Status: Active
Source Idea: ideas/open/deferred_nttp_expr_parser_gaps.md
Source Plan: plan.md

## Current Active Item

- Step 4: end-to-end validation and leftover gaps

## Current Slice

- Rebuild clean, run the full suite, and verify the new static-member slice is monotonic against the recorded baseline
- Record the next planned Step 3 `sizeof...(pack)` slice after validation

## Completed

- Activated [plan.md](/workspaces/c4c/plan.md) from `ideas/open/deferred_nttp_expr_parser_gaps.md`
- Added `cpp_hir_template_deferred_nttp_arith_expr` coverage for `M = N + 2`
- Fixed deferred NTTP token evaluation to handle additive and multiplicative arithmetic during parser-side template instantiation
- Verified the targeted deferred NTTP HIR regression set passes
- Verified clean rebuild full-suite results are monotonic: `2227` passed after vs `2226` before, with `0` new failing tests
- Added `cpp_hir_template_deferred_nttp_static_member_expr` coverage for `Count<N>::value + 2`
- Fixed HIR global declaration lowering to resolve pending template struct types before emitting global variable types
- Hardened HIR static-member evaluation to search both stored declaration lists and template origins when resolving constexpr members
- Verified the targeted Step 2 HIR regression trio passes: arithmetic, alias static-member, and direct static-member deferred defaults
- Verified clean rebuild full-suite results are monotonic after the Step 2 static-member slice: `2235` total / `2228` passed / `7` failed after vs `2234` total / `2227` passed / `7` failed before, with `0` new failing tests

## Next Intended Slice

- Start Step 3 with a focused `sizeof...(Ts)` deferred default HIR regression once the full-suite validation for Step 2 is recorded

## Blockers

- None recorded

## Resume Notes

- The arithmetic failure was blocked earlier than HIR: parser-side deferred NTTP token evaluation lacked `*`, `/`, `+`, and `-`, so template instantiation produced `M_0` before HIR lowering.
- Step 2 should target a direct class-template default expression, not just the existing alias-template trait case already covered by `cpp_hir_template_alias_deferred_nttp_static_member`.
- The direct static-member slice depended on HIR, not the parser: unresolved global template types were skipping pending-template resolution in `lower_global`, leaving `Buffer_N_3_M_0` untouched.
- `test_fail_before.log` vs `test_fail_after.log` passed the monotonic guard with `passed +2`, `failed +0`, and `0` newly failing tests; the persistent failures remain the same seven pre-existing cases.
- Existing persistent full-suite failures remain the template/sema cluster plus the separate `initializer_list` runtime case; no new failures were introduced by this slice.
