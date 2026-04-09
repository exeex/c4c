# Active Todo

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Source Plan: plan.md

## Active Item

- Step 1: audit reference-qualified cast targets
- Current slice: widen the casted-reference member-access matrix from the now
  covered `(T&)expr` field-access case to the first `&&` or inherited/base
  access shape that still lacks explicit regression coverage
- Current implementation target: `tests/cpp` cast/reference runtime regressions
  plus the earliest failing parser, sema, HIR, or lowering surface the first
  unresolved casted-member-access case exposes
- Next intended slice: add the narrowest `(T&&)expr` field-access or inherited
  member-access case, compare against Clang when value category matters, and
  stop at the earliest failing stage if it does not already pass

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
- Added `tests/cpp/internal/postive_case/c_style_cast_ref_qualified_method.cpp`
  to cover ref-qualified method dispatch through `(Probe&)p` and `(Probe&&)p`.
- Fixed parser/HIR method ref-overload tracking so member `&`/`&&` qualifiers
  survive into overload resolution on the implicit object.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2843/2843
  passed, `test_fail_after.log` 2844/2844 passed, with zero new failures.
- Added `tests/cpp/internal/postive_case/c_style_cast_lvalue_ref_field_access.cpp`
  to cover field read/write through `(Box&)b`.
- Compared the new field-access case against Clang and confirmed that parser,
  HIR, lowering, and runtime behavior already match for the `(T&)expr`
  lvalue-reference member-access slice, so no compiler change was needed.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2843/2843
  passed, `test_fail_after.log` 2845/2845 passed, with zero new failures.

## Notes

- Clang lowers the `(volatile int&)x` writeback as a `volatile` store, while
  current `c4cll` IR emits a plain store. The runtime regression still passes,
  so this is recorded as a later-stage qualifier/lowering follow-up rather than
  the first Step 1 parser/sema failure.
- The ref-qualified method failure was a HIR overload-selection issue for the
  implicit object: method ref-overload metadata only tracked explicit
  parameters, so zero-arg `&`/`&&` methods always selected the first overload
  until the parser and overload set started carrying method ref qualifiers.
- The `(T&)expr` field-access probe did not expose a frontend or HIR bug: HIR
  preserves `((struct Box&)b).value` as a field lvalue on the original object,
  and the emitted LLVM IR lowers both read and write through the same storage
  as Clang.
