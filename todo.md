# Active Todo

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Source Plan: plan.md

## Active Item

- Step 1: audit reference-qualified cast targets
- Current slice: extend Step 1 beyond free-function overload selection into the
  next value-category-sensitive reference-cast case, likely method dispatch or
  mutation through a casted reference
- Current implementation target: `tests/cpp` cast/reference runtime regressions
  plus the earliest failing frontend or HIR surface the next case exposes
- Next intended slice: add the first focused value-category-sensitive
  reference-cast case for member/method behavior through `(T&)expr` or
  `(T&&)expr`, compare against Clang, and classify any mismatch by earliest
  failing stage before widening coverage further

## Completed

- Activated `ideas/open/43_c_style_cast_reference_followups_review.md` into the
  active runbook after closing idea 42
- Added `tests/cpp/internal/postive_case/c_style_cast_cv_lvalue_ref_basic.cpp`
  to cover `(const int&)x` readback and `(volatile int&)x` writeback.
- Confirmed the new cv-qualified lvalue-reference cast regression passes along
  with the existing `c_style_cast_lvalue_ref_basic` and
  `c_style_cast_rvalue_ref_basic` cases.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2841/2841
  passed, `test_fail_after.log` 2842/2842 passed, with zero new failures.
- Isolated the first value-category-sensitive failure with a reduced overload
  probe: sema classified `(int&)x` as an lvalue, but HIR ref-overload
  selection treated `NK_CAST` as non-lvalue and incorrectly picked the `int&&`
  overload.
- Added `tests/cpp/internal/postive_case/c_style_cast_lvalue_ref_overload.cpp`
  to cover free-function overload selection through `(int&)x` and `(int&&)x`.
- Fixed HIR ref-overload selection so `NK_CAST` nodes targeting lvalue
  references are classified as lvalues.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2842/2842
  passed, `test_fail_after.log` 2843/2843 passed, with zero new failures.

## Notes

- Clang lowers the `(volatile int&)x` writeback as a `volatile` store, while
  current `c4cll` IR emits a plain store. The runtime regression still passes,
  so this is recorded as a later-stage qualifier/lowering follow-up rather than
  the first Step 1 parser/sema failure.
