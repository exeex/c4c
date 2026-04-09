# Active Todo

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Source Plan: plan.md

## Active Item

- Step 1: audit reference-qualified cast targets
- Current slice: extend Step 1 from basic `int&` / `int&&` coverage into the
  first value-category-sensitive reference-cast regression after landing the
  missing cv-qualified lvalue-reference runtime case
- Current implementation target: `tests/cpp` cast/reference regressions plus
  the earliest failing parser/sema surface those tests expose
- Next intended slice: add the first focused value-category-sensitive
  reference-cast case (assignment, overload selection, or mutation through a
  casted reference) that exposes an unresolved mismatch and classify its
  earliest failing stage with Clang comparison

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

## Notes

- Clang lowers the `(volatile int&)x` writeback as a `volatile` store, while
  current `c4cll` IR emits a plain store. The runtime regression still passes,
  so this is recorded as a later-stage qualifier/lowering follow-up rather than
  the first Step 1 parser/sema failure.
