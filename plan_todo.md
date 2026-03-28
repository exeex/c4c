# Deferred NTTP Expression Parser Gap Todo

Status: Active
Source Idea: ideas/open/deferred_nttp_expr_parser_gaps.md
Source Plan: plan.md

## Current Active Item

- Step 4: end-to-end validation and leftover gaps

## Current Slice

- Record the completed `sizeof...(Ts)` deferred-default slice and the clean full-suite monotonic result
- Leave the next iteration positioned to review any remaining deferred-expression shapes that are still outside the documented arithmetic, static-member, and pack-size set

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
- Added `cpp_hir_template_deferred_nttp_sizeof_pack_expr` coverage for a deferred `sizeof...(Ts)` default that materializes `int data[3]`
- Deferred pack-bearing template-struct type applications from parser type parsing into the HIR pending-template path so pack defaults keep their explicit pack arguments intact
- Extended HIR deferred template-argument materialization and mangling to preserve pack bindings and evaluate a missing NTTP default from `sizeof...(Ts)`
- Fixed a compile-time engine pending-template worklist invalidation by copying the current work item before callbacks append more pending items
- Verified the targeted deferred NTTP HIR regression set passes with the new pack-size case included
- Verified clean rebuild full-suite results are monotonic after the Step 3 pack-size slice: `2236` total / `2229` passed / `7` failed after vs `2235` total / `2228` passed / `7` failed before, with `0` new failing tests

## Next Intended Slice

- Review whether any leftover deferred NTTP expression gaps remain outside the three documented mechanism slices and record them explicitly instead of expanding the runbook ad hoc

## Blockers

- None recorded

## Resume Notes

- The arithmetic failure was blocked earlier than HIR: parser-side deferred NTTP token evaluation lacked `*`, `/`, `+`, and `-`, so template instantiation produced `M_0` before HIR lowering.
- Step 2 should target a direct class-template default expression, not just the existing alias-template trait case already covered by `cpp_hir_template_alias_deferred_nttp_static_member`.
- The direct static-member slice depended on HIR, not the parser: unresolved global template types were skipping pending-template resolution in `lower_global`, leaving `Buffer_N_3_M_0` untouched.
- `test_fail_before.log` vs `test_fail_after.log` passed the monotonic guard with `passed +2`, `failed +0`, and `0` newly failing tests; the persistent failures remain the same seven pre-existing cases.
- Existing persistent full-suite failures remain the template/sema cluster plus the separate `initializer_list` runtime case; no new failures were introduced by this slice.
- The source-idea repro for Step 3 uses a non-standard template-parameter ordering that Clang rejects; this slice keeps it as an internal HIR regression and defers pack-bearing template-struct type parsing to HIR rather than treating it as a Clang-conformance target.
- The new pack-size path exposed a pre-existing compile-time-engine bug: pending template type callbacks could append to the worklist and invalidate the current element reference mid-iteration.
