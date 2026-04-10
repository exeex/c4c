# HIR Function-Return Temporary Member Call Lowering

Status: Complete
Last Updated: 2026-04-10

## Goal

Fix HIR lowering for ref-qualified member calls on function-return temporaries
 so expressions such as `make_box().read()` lower as one temporary-producing
 call followed by the correct member dispatch, instead of being misrouted
 through unrelated call-lowering paths.

## Why This Idea Exists

While validating the HIR helper-extraction slice, a targeted regression case
showed that function-return temporary member calls still lower incorrectly.
The helper-extraction work stayed behavior-preserving by narrowing the new test
to already-supported seams, but this remains a real frontend/HIR issue that
deserves its own plan.

## Scope

- `src/frontend/hir/hir_expr.cpp`
- nearby HIR helper files if the fix naturally belongs there
- targeted runtime or HIR-dump regression coverage for temporary member-call
  dispatch

## Expected Validation

- focused frontend/HIR regression covering `function().method()` on
  ref-qualified methods
- nearby temporary/member/operator call regressions to prove no dispatch
  regressions
- full-suite regression check before landing

## Completion Notes

- Completed 2026-04-10.
- Fixed HIR member-call lowering in `src/frontend/hir/hir_expr_call.cpp` so a
  non-lvalue function-call base is first materialized into a temporary local
  before the implicit `this` pointer is formed.
- Added runtime coverage in
  `tests/cpp/internal/postive_case/function_return_temporary_member_call_runtime.cpp`.
- Added HIR dump coverage in
  `tests/cpp/internal/hir_case/function_return_temporary_member_call_hir.cpp`
  together with an explicit dump-shape assertion in
  `tests/cpp/internal/InternalTests.cmake`.
- Validation passed for the focused new tests and for a full-suite monotonic
  regression guard:
  `before passed=3361 failed=0 total=3361`,
  `after passed=3363 failed=0 total=3363`.

## Leftover Issues

- None from this slice.
