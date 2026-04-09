# Active Todo

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Source Plan: plan.md

## Active Item

- Step 2: review typedef, alias, and qualified cast targets
- Current slice: continue Step 2 from the now-green nested dependent
  template-owner alias case to the next distinct qualified/dependent
  alias-owned cast shape
- Current implementation target: find the next smallest Step 2 reduction that
  is not already covered by plain dependent `typename`, nested dependent
  aliases, or the dependent `::template` owner chain
- Next intended slice: keep Step 2 focused on one new alias-owned qualified or
  dependent cast target at a time; do not broaden into declarator-suffix work
  until a new uncovered shape is identified

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
- Added `tests/cpp/internal/postive_case/c_style_cast_rvalue_ref_field_access.cpp`
  to cover `((Box&&)b).value` readback, rvalue-ref overload selection, and
  binding an `int&&` directly to the casted field.
- Compared the new `((Box&&)b).value` case against Clang and confirmed the
  intended xvalue behavior: overload resolution picks `int&&`, and the bound
  reference aliases the original field storage rather than a temporary copy.
- Fixed sema/HIR value-category propagation so member access through an rvalue
  reference cast is no longer forced to lvalue, and rvalue-reference binding
  reuses existing member storage instead of materializing a temporary.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2845/2845
  passed, `test_fail_after.log` 2846/2846 passed, with zero new failures.
- Added `tests/cpp/internal/postive_case/c_style_cast_base_lvalue_ref_field_access.cpp`
  to cover read/write through `((Base&)derived).value` without depending on
  broader inherited-layout initialization.
- Compared the scoped casted-base field-access case against Clang and confirmed
  that the read/write behavior for `((Base&)derived).value` matches the
  intended baseline, so no cast-specific compiler change was needed for this
  slice.
- Recorded the broader inherited-record layout and plain base-member lookup gap
  in `ideas/open/48_inherited_record_layout_and_base_member_access_followup.md`
  instead of expanding the active cast-follow-up plan.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2845/2845
  passed, `test_fail_after.log` 2847/2847 passed, with zero new failures.
- Added `tests/cpp/internal/postive_case/c_style_cast_base_rvalue_ref_field_access.cpp`
  to cover `((Base&&)d).value` readback, overload selection, and `int&&`
  binding through a casted base subobject.
- Compared the new `((Base&&)d).value` case against Clang and confirmed the
  intended xvalue behavior: overload resolution picks `int&&`, and the bound
  reference aliases the original base-subobject storage rather than a
  temporary.
- Fixed HIR record-layout computation so structs with non-virtual base
  subobjects reserve inherited storage and alignment before their own fields,
  which prevents base-only derived objects from lowering as zero-sized storage
  during casted base-reference field access.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2847/2847
  passed, `test_fail_after.log` 2848/2848 passed, with zero new failures.
- Added `tests/cpp/internal/postive_case/c_style_cast_typedef_ref_alias_basic.cpp`
  to cover `using`/`typedef`-spelled `int&` and `int&&` cast targets,
  assignment through the aliased references, and overload selection on the
  cast expressions themselves.
- Confirmed the alias-owned reference-cast runtime slice already matches Clang:
  the targeted regression passed without compiler changes, including lvalue and
  rvalue-reference overload selection through `(AliasL)x` and `(AliasR)x`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2848/2848
  passed, `test_fail_after.log` 2849/2849 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_qualified_typedef_ref_alias_basic.cpp`
  to cover namespace-qualified `ns::AliasL` / `ns::AliasR` cast targets,
  assignment through the aliased references, and overload selection on the
  cast expressions themselves.
- Confirmed the namespace-qualified alias-owned reference-cast runtime slice
  already matches Clang: the targeted regression passed without compiler
  changes, including lvalue and rvalue-reference overload selection through
  `(ns::AliasL)x` and `(ns::AliasR)x`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2848/2848
  passed, `test_fail_after.log` 2850/2850 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_member_typedef_ref_alias_basic.cpp`
  to cover member-owned `Box::AliasL` / `Box::AliasR` cast targets, assignment
  through the aliased references, and overload selection on the cast
  expressions themselves.
- Confirmed the member-owned alias reference-cast runtime slice already matches
  Clang: the targeted regression passed without compiler changes, including
  lvalue and rvalue-reference overload selection through `(Box::AliasL)x` and
  `(Box::AliasR)x`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2850/2850
  passed, `test_fail_after.log` 2851/2851 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_global_qualified_typedef_ref_alias_basic.cpp`
  to cover global-qualified `::ns::AliasL` / `::ns::AliasR` cast targets,
  assignment through the aliased references, and overload selection on the
  cast expressions themselves.
- Confirmed the global-qualified namespace-alias reference-cast runtime slice
  already matches Clang: the targeted regression passed without compiler
  changes, including lvalue and rvalue-reference overload selection through
  `(::ns::AliasL)x` and `(::ns::AliasR)x`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2851/2851
  passed, `test_fail_after.log` 2852/2852 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_dependent_typename_ref_alias_basic.cpp`
  to cover dependent `(typename Holder<T>::AliasL)x` /
  `(typename Holder<T>::AliasR)x` cast targets, assignment through the aliased
  references, and overload selection on the cast expressions themselves.
- Compared the dependent `typename` alias-owned reference-cast slice against
  Clang and confirmed the first reduction already matches parser, HIR, and
  runtime expectations, so no compiler change was needed for this slice.
- Full-suite validation stayed monotonic: `test_before.log` 2852/2852 passed,
  `test_after.log` 2853/2853 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_nested_dependent_typename_ref_alias_basic.cpp`
  to cover nested dependent
  `(typename Holder<T>::Inner::AliasL)x` /
  `(typename Holder<T>::Inner::AliasR)x` cast targets, assignment through the
  aliased references, and overload selection on the cast expressions
  themselves.
- Fixed dependent nested member-typedef resolution so parser and HIR preserve
  reference qualifiers across `typename Holder<T>::Inner::AliasL` /
  `AliasR`, including the lvalue/rvalue category used by overload selection on
  the cast expressions.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2853/2853
  passed, `test_fail_after.log` 2854/2854 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_dependent_template_member_ref_alias_basic.cpp`
  to cover dependent
  `(typename Holder<T>::template Rebind<T>::AliasL)x` /
  `(typename Holder<T>::template Rebind<T>::AliasR)x` cast targets,
  assignment through the aliased references, and overload selection on the
  cast expressions themselves.
- Compared the dependent `::template` member-alias reference-cast slice against
  Clang-backed expectations and confirmed that parser, HIR, runtime behavior,
  and overload selection already match for this reduction, so no compiler
  change was needed for the slice.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2854/2854
  passed, `test_fail_after.log` 2855/2855 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_nested_dependent_template_member_ref_alias_basic.cpp`
  to cover nested dependent
  `(typename Holder<T>::template Rebind<T>::Inner::AliasL)x` /
  `(typename Holder<T>::template Rebind<T>::Inner::AliasR)x` cast targets,
  assignment through the aliased references, and overload selection on the
  cast expressions themselves.
- Compared the nested dependent `::template` owner-chain reference-cast slice
  against Clang-backed expectations and confirmed that parser, HIR, runtime
  behavior, and overload selection already match for this reduction, so no
  compiler change was needed for the slice.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2855/2855
  passed, `test_fail_after.log` 2856/2856 passed, with zero new failures.

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
- The first `((T&&)expr).field` draft tried direct assignment and Clang rejected
  it as non-assignable, so the landed regression checks xvalue-sensitive
  behavior through overload selection plus `int&&` binding instead of treating
  `((Box&&)b).value = ...` as the reference baseline.
- The first draft of the casted-base-member regression also checked
  `Derived d{{1}};` and `d.value`, which exposed a broader inherited-layout and
  plain base-member lookup problem. That issue was spun out into
  `ideas/open/48_inherited_record_layout_and_base_member_access_followup.md`
  so the current plan stays focused on cast-specific behavior.
- The landed `((Base&&)d).value` regression initially crashed because HIR
  record layout treated `Derived : Base {}` as an empty record. Folding
  inherited base-subobject size/alignment into record layout fixed the
  cast-blocking runtime issue without absorbing the broader plain
  inherited-member lookup follow-up into this plan.
- The first dependent `typename` alias-owned reference-cast reduction did not
  expose a new parser, sema, HIR, or lowering failure: `c4cll --parse-only`,
  `--dump-hir`, and the runtime regression all aligned with the Clang-backed
  expectation for `(typename Holder<T>::AliasL)x` and
  `(typename Holder<T>::AliasR)x`.
- The nested dependent alias-owned follow-up initially exposed two linked
  issues: parser-side dependent member-typedef resolution did not synthesize
  nested owner chains such as `Holder<T>::Inner::AliasL`, and HIR’s fallback
  member-typedef substitution path dropped outer `&` / `&&` qualifiers when it
  substituted the inner alias’s `T` binding.
- The dependent `::template` member-alias reduction did not expose a new
  parser, sema, HIR, or lowering mismatch: the focused runtime regression and
  full-suite check both stayed green for
  `typename Holder<T>::template Rebind<T>::AliasL` /
  `AliasR`.
- The nested dependent `::template` owner-chain reduction also stayed green:
  `typename Holder<T>::template Rebind<T>::Inner::AliasL` / `AliasR` parsed,
  lowered, and ran with the same lvalue/rvalue overload behavior Clang showed,
  so Step 2 still needs a different uncovered qualified/dependent alias shape
  rather than a fix in this lane.
