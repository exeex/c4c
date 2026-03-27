# Deferred NTTP Expression Parser Gap Todo

Status: Active
Source Idea: ideas/open/deferred_nttp_expr_parser_gaps.md
Source Plan: plan.md

## Current Active Item

- Step 2: template static-member lookup in deferred NTTP defaults

## Current Slice

- Add a focused internal `--dump-hir` regression for `Count<N>::value + 2`
- Extend deferred NTTP evaluation through the next missing static-member mechanism without broad template refactors

## Completed

- Activated [plan.md](/workspaces/c4c/plan.md) from `ideas/open/deferred_nttp_expr_parser_gaps.md`
- Added `cpp_hir_template_deferred_nttp_arith_expr` coverage for `M = N + 2`
- Fixed deferred NTTP token evaluation to handle additive and multiplicative arithmetic during parser-side template instantiation
- Verified the targeted deferred NTTP HIR regression set passes
- Verified clean rebuild full-suite results are monotonic: `2227` passed after vs `2226` before, with `0` new failing tests

## Next Intended Slice

- Add the dependent static-member lookup case `Count<N>::value + 2`

## Blockers

- None recorded

## Resume Notes

- The arithmetic failure was blocked earlier than HIR: parser-side deferred NTTP token evaluation lacked `*`, `/`, `+`, and `-`, so template instantiation produced `M_0` before HIR lowering.
- Existing persistent full-suite failures remain the template/sema cluster plus the separate `initializer_list` runtime case; no new failures were introduced by this slice.
