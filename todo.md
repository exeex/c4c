# Active Todo

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Source Plan: plan.md

## Active Item

- Step 4: validate downstream value-category behavior
- Step 4: validate downstream value-category behavior
- Current slice: audit whether Step 4 still has an uncovered decltype-like
  consumer after the existing overload, forwarding, and member-access slices
- Current implementation target: probe the narrowest decltype-like cast
  consumer that can still disagree with Clang on `(T&)expr` / `(T&&)expr`
  category preservation
- Next intended slice: add the smallest decltype-like Step 4 regression if one
  is still uncovered; otherwise record Step 4 as coverage-complete and hand off
  to Step 5 casted member/base access review

## Completed

- Added
  `tests/cpp/internal/postive_case/c_style_cast_forwarding_ref_overload.cpp`
  as a focused Step 4 regression for a forwarding-reference template consumer
  that calls `pick((T&&)value)` after a C-style cast.
- Fixed HIR free-function ref-overload resolution so dependent cast arguments
  consult the active template bindings when deciding whether a cast expression
  is an lvalue, which restores reference-collapsing behavior for the
  `forward_pick<int&>((int&)x)` slice.
- Targeted validation passed:
  `ctest --test-dir build -R 'c_style_cast_(forwarding_ref_overload|lvalue_ref_overload|ref_qualified_method|rvalue_ref_field_access|base_rvalue_ref_field_access)' --output-on-failure`.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2873/2873 passed,
  `test_fail_after.log` 2875/2875 passed, with zero new failures.

- Added
  `tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_template_member_fn_ptr_const_parse.cpp`
  as a focused parse-only regression for the namespace-qualified non-dependent
  owner form `(int (ns::Box<int>::*)(int) const)0` inside a C-style cast
  target.
- Confirmed the remaining Step 3 non-dependent qualified-owner `const`
  member-function-pointer cast slice already matches the parser baseline in the
  current tree, so no compiler change was needed for this regression-only
  slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_template_member_fn_ptr_const_parse.cpp`,
  `ctest --test-dir build -R 'c_style_cast_(namespace_qualified_template_member_fn_ptr_const_parse|namespace_qualified_template_member_fn_ptr_ref_qual_parse|global_qualified_template_member_fn_ptr_const_parse|template_member_fn_ptr_const_parse|template_member_fn_ptr_ref_qual_parse)' --output-on-failure`,
  and `ctest --test-dir build -N` confirms the new test is registered under
  `CPP_POSITIVE_PARSE_STEMS`.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2873/2873 passed,
  `test_fail_after.log` 2874/2874 passed, with zero new failures.

- Added
  `tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_template_member_fn_ptr_ref_qual_parse.cpp`
  as a focused parse-only regression for the namespace-qualified template-id
  owner form `(int (ns::Box<int>::*)(int) &)0` / `&&` with trailing
  member-function ref-qualifiers inside a C-style cast target.
- Confirmed the new namespace-qualified non-dependent owner trailing
  `&` / `&&` member-function-pointer cast slice already matches the parser
  baseline in the current tree, so no compiler change was needed for this
  regression-only slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_template_member_fn_ptr_ref_qual_parse.cpp`,
  `ctest --test-dir build -R 'c_style_cast_(namespace_qualified_template_member_fn_ptr_ref_qual_parse|namespace_qualified_dependent_template_member_fn_ptr_ref_qual_parse|global_qualified_template_member_fn_ptr_const_parse|template_member_fn_ptr_ref_qual_parse)' --output-on-failure`,
  and `ctest --test-dir build -N` confirms the new test is registered under
  `CPP_POSITIVE_PARSE_STEMS`.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2872/2872 passed,
  `test_fail_after.log` 2873/2873 passed, with zero new failures.

- Added
  `tests/cpp/internal/postive_case/c_style_cast_global_qualified_dependent_template_member_fn_ptr_ref_qual_parse.cpp`
  as a focused parse-only regression for the global-qualified dependent
  `template` owner chain
  `(int (::ns::Holder<T>::template Rebind<T>::Inner::*)(T) &)0` /
  `&&` with trailing member-function ref-qualifiers inside a C-style cast
  target.
- Confirmed the new global-qualified dependent owner-chain trailing
  `&` / `&&` member-function-pointer cast slice already matches the parser
  baseline in the current tree, so no compiler change was needed for this
  regression-only slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_global_qualified_dependent_template_member_fn_ptr_ref_qual_parse.cpp`,
  `ctest --test-dir build -R '^cpp_positive_sema_c_style_cast_global_qualified_dependent_template_member_fn_ptr_ref_qual_parse_cpp$' --output-on-failure`,
  and `ctest --test-dir build -N` confirms the new test is registered under
  `CPP_POSITIVE_PARSE_STEMS`.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2871/2871 passed,
  `test_fail_after.log` 2872/2872 passed, with zero new failures.

- Added
  `tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_dependent_template_member_fn_ptr_ref_qual_parse.cpp`
  as a focused parse-only regression for the namespace-qualified dependent
  `template` owner chain
  `(int (ns::Holder<T>::template Rebind<T>::Inner::*)(T) &)0` /
  `&&` with trailing member-function ref-qualifiers inside a C-style cast
  target.
- Confirmed the new namespace-qualified dependent owner-chain trailing
  `&` / `&&` member-function-pointer cast slice already matches the parser
  baseline in the current tree, so no compiler change was needed for this
  regression-only slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_dependent_template_member_fn_ptr_ref_qual_parse.cpp`
  and
  `ctest --test-dir build -R 'c_style_cast_(namespace_qualified_dependent_template_member_fn_ptr_ref_qual_parse|namespace_qualified_dependent_template_member_fn_ptr_const_parse|global_qualified_dependent_template_member_fn_ptr_const_parse|template_member_fn_ptr_ref_qual_parse)' --output-on-failure`.
- Registered the new regression under `CPP_POSITIVE_PARSE_STEMS` so the
  internal harness continues treating the file as a parse-only Step 3 probe.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2869/2869 passed,
  `test_fail_after.log` 2871/2871 passed, with zero new failures.

- Added
  `tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_dependent_template_member_fn_ptr_const_parse.cpp`
  to cover the namespace-qualified dependent `template` owner chain
  `(int (ns::Holder<T>::template Rebind<T>::Inner::*)(T) const)0` inside a
  parenthesized member-function-pointer C-style cast target.
- Ran the new namespace-qualified dependent owner-chain parser regression
  alongside the adjacent global-qualified/template member-function-pointer
  cast tests and confirmed the slice is coverage-only; parser acceptance
  already matches the existing matrix, so no compiler change was needed.
- Full-suite validation stayed monotonic: `test_before.log` 2869/2869 passed,
  `test_after.log` 2870/2870 passed, with zero new failures.
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
  in `ideas/open/45_inherited_record_layout_and_base_member_access_followup.md`
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
- Added
  `tests/cpp/internal/postive_case/c_style_cast_namespace_qualified_dependent_typename_ref_alias_basic.cpp`
  to cover namespace-qualified dependent
  `(typename ns::Holder<T>::AliasL)x` /
  `(typename ns::Holder<T>::AliasR)x` cast targets, assignment through the
  aliased references, and overload selection on the cast expressions
  themselves.
- Fixed parser-side dependent `typename` member-typedef resolution so
  namespace-qualified owner prefixes are folded into the lookup before HIR
  lowering. That restores the underlying `T&` / `T&&` cast target on the AST
  node itself, which in turn preserves lvalue/rvalue classification and avoids
  the bad temporary materialization path seen in HIR/runtime.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2856/2856
  passed, `test_fail_after.log` 2857/2857 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_global_qualified_nested_dependent_typename_ref_alias_basic.cpp`
  to cover global-qualified nested dependent
  `(typename ::ns::Holder<T>::Inner::AliasL)x` /
  `(typename ::ns::Holder<T>::Inner::AliasR)x` cast targets, assignment
  through the aliased references, and overload selection on the cast
  expressions themselves.
- Fixed parser-side dependent `typename` nested-owner lookup so
  global-qualified namespaced owner chains match nested record members by
  source spelling rather than only their canonicalized namespace-qualified
  declaration names, which restores the underlying `T&` / `T&&` cast target
  before HIR lowering.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2857/2857
  passed, `test_fail_after.log` 2858/2858 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_global_qualified_nested_dependent_template_member_ref_alias_basic.cpp`
  to cover the mixed global-qualified namespace-plus-`template` owner chain
  `(typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasL)x` /
  `AliasR`, including assignment through the aliased references and overload
  selection on the cast expressions.
- Compared the mixed global-qualified namespace-plus-`template` reference-cast
  slice against Clang and confirmed that parser, HIR, runtime behavior, and
  overload selection already match for this reduction, so no compiler change
  was needed for the final Step 2 alias-owned qualified/dependent case.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2858/2858
  passed, `test_fail_after.log` 2859/2859 passed, with zero new failures.
- Isolated the first Step 3 parser hole with reduced `--parse-only` probes:
  plain template-id pointer casts such as `(Box<int>* const)raw` already
  parse, but a template-id return type inside a function-pointer declarator
  like `Box<int> (*fp)(int) = (Box<int> (*)(int))raw;` is misclassified as an
  expression statement before the cast target can be parsed.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_return_type_parse.cpp`
  as a parse-only regression for `Box<int> (*fp)(int) = (Box<int> (*)(int))raw;`.
- Fixed the parser’s value-like template-expression heuristic so
  `Identifier<...>(*)` / `(&)` / `(&&)` heads stay eligible for local
  declaration parsing instead of being forced down the expression-statement
  path before Step 3 function-pointer cast targets can be recognized.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_return_type_parse.cpp`
  and `ctest --test-dir build -R c_style_cast_template_fn_ptr_return_type_parse --output-on-failure`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2858/2858
  passed, `test_fail_after.log` 2860/2860 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_param_type_parse.cpp`
  as a focused parse-only regression for
  `int (*fp)(Box<int>) = (int (*)(Box<int>))raw;`.
- Confirmed the Step 3 template-id parameter-type variant already matches the
  parser baseline in the current tree, so no compiler change was needed for
  this slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_param_type_parse.cpp`
  and `ctest --test-dir build -R c_style_cast_template_fn_ptr_param_type_parse --output-on-failure`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2860/2860
  passed, `test_fail_after.log` 2861/2861 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_member_fn_ptr_const_parse.cpp`
  as a focused parse-only regression for
  `int (Box<int>::*fp)(int) const = (int (Box<int>::*)(int) const)raw;`.
- Confirmed the Step 3 template-id member-function-pointer `const` variant
  already matches the parser baseline in the current tree, so no compiler
  change was needed for this slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_member_fn_ptr_const_parse.cpp`
  and `ctest --test-dir build -R c_style_cast_template_member_fn_ptr_const_parse --output-on-failure`.
- Full-suite validation stayed monotonic: `test_fail_before.log` 2860/2860
  passed, `test_fail_after.log` 2862/2862 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_member_fn_ptr_ref_qual_parse.cpp`
  as a focused parse-only regression for
  `int (Box<int>::*fp)(int) &` /
  `int (Box<int>::*fp)(int) &&` cast targets with template-id owners.
- Fixed the parser’s parenthesized function-pointer suffix path so trailing
  member-function ref-qualifiers after the parameter list are consumed during
  C-style cast type parsing instead of leaving `=` to start a misclassified
  expression statement.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_member_fn_ptr_ref_qual_parse.cpp`,
  `ctest --test-dir build -R c_style_cast_template_member_fn_ptr_ref_qual_parse --output-on-failure`,
  and nearby Step 3 parser cases for the return-type, member-`const`, and
  member-function-pointer suffix variants.
- Full-suite validation stayed monotonic: `test_before.log` 2862/2862 passed,
  `test_after.log` 2863/2863 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse.cpp`
  as a focused parse-only regression for
  `Box<int>&& (*fp)(int) = (Box<int>&& (*)(int))raw;`.
- Fixed the expression-side C-style cast template-id lookahead so
  `Identifier<...>&&` remains eligible for type-id parsing when a nested
  function-pointer declarator follows, instead of being rejected as an
  expression-shaped template head before `parse_type_name()` can consume the
  cast target.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse.cpp`,
  `ctest --test-dir build -R 'c_style_cast_template_(fn_ptr_return_type_parse|fn_ptr_rvalue_ref_return_type_parse|member_fn_ptr_ref_qual_parse|member_fn_ptr_const_parse|fn_ptr_param_type_parse)' --output-on-failure`,
  and nearby Step 3 parser probes for the lvalue-reference return-type,
  member-`const`, and member-function-pointer ref-qualifier variants.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2860/2860 passed,
  `test_fail_after.log` 2864/2864 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_member_ptr_parse.cpp`
  as a focused parse-only regression for
  `int Box<int>::*member = (int Box<int>::*)0;`.
- Confirmed the adjacent Step 3 template-id data-member-pointer cast variant
  already matches the parser baseline in the current tree, so no compiler
  change was needed for this slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_member_ptr_parse.cpp`
  and `ctest --test-dir build -R c_style_cast_template_member_ptr_parse --output-on-failure`.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2860/2860 passed,
  `test_fail_after.log` 2865/2865 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_lvalue_ref_return_type_parse.cpp`
  as a focused parse-only regression for
  `Box<int>& (*fp)(int) = (Box<int>& (*)(int))raw;`.
- Confirmed the adjacent Step 3 template-id lvalue-reference function-pointer
  return-type cast variant already matches the parser baseline in the current
  tree, so no compiler change was needed for this slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_template_fn_ptr_lvalue_ref_return_type_parse.cpp`
  and
  `ctest --test-dir build -R 'c_style_cast_template_(fn_ptr_return_type_parse|fn_ptr_lvalue_ref_return_type_parse|fn_ptr_rvalue_ref_return_type_parse|member_fn_ptr_ref_qual_parse|member_fn_ptr_const_parse|fn_ptr_param_type_parse|member_ptr_parse)' --output-on-failure`.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2865/2865 passed,
  `test_fail_after.log` 2866/2866 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_dependent_template_member_ptr_parse.cpp`
  as a focused parse-only regression for the dependent owner-chain
  `int Holder<T>::Type::*member = (int Holder<T>::Type::*)0;`.
- Fixed parser-side member-pointer owner-prefix detection so
  `consume_member_pointer_owner_prefix()` accepts qualified owner chains with
  template-ids on non-final segments, which keeps dependent
  `Holder<T>::Type::*` and the adjacent member-function-pointer form on the
  type-id path during C-style cast parsing.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_dependent_template_member_ptr_parse.cpp`,
  `ctest --test-dir build -R 'c_style_cast_(dependent_template_member_ptr_parse|template_member_ptr_parse|template_member_fn_ptr_ref_qual_parse|template_member_fn_ptr_const_parse|template_fn_ptr_lvalue_ref_return_type_parse|template_fn_ptr_rvalue_ref_return_type_parse|template_fn_ptr_return_type_parse)' --output-on-failure`,
  and an ad-hoc parse-only probe for the corresponding
  `int (Holder<T>::Type::*fp)(T) &` cast target.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2866/2866 passed,
  `test_fail_after.log` 2867/2867 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_global_qualified_template_member_fn_ptr_const_parse.cpp`
  as a focused parse-only regression for
  `int (::ns::Box<int>::*fp)(int) const =
  (int (::ns::Box<int>::*)(int) const)raw;`.
- Confirmed the next Step 3 qualified-owner member-function-pointer cast slice
  already matches the parser baseline in the current tree, so no compiler
  change was needed for this regression-only slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_global_qualified_template_member_fn_ptr_const_parse.cpp`
  and
  `ctest --test-dir build -R 'c_style_cast_(global_qualified_template_member_fn_ptr_const_parse|dependent_template_member_ptr_parse|template_member_fn_ptr_const_parse|template_member_fn_ptr_ref_qual_parse|template_member_ptr_parse)' --output-on-failure`.
- Registered the new regression under `CPP_POSITIVE_PARSE_STEMS` so the
  internal harness continues treating the file as a parse-only Step 3 probe
  instead of a compile/runtime positive case.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2867/2867 passed,
  `test_fail_after.log` 2868/2868 passed, with zero new failures.
- Added
  `tests/cpp/internal/postive_case/c_style_cast_global_qualified_dependent_template_member_fn_ptr_const_parse.cpp`
  as a focused parse-only regression for the global-qualified dependent
  `template` owner chain
  `(int (::ns::Holder<T>::template Rebind<T>::Inner::*)(T) const)0`.
- Confirmed the new Step 3 global-qualified dependent member-function-pointer
  cast slice already matches the parser baseline in the current tree, so no
  compiler change was needed for this regression-only slice.
- Targeted validation passed:
  `build/c4cll --parse-only tests/cpp/internal/postive_case/c_style_cast_global_qualified_dependent_template_member_fn_ptr_const_parse.cpp`
  and
  `ctest --test-dir build -R 'c_style_cast_(global_qualified_dependent_template_member_fn_ptr_const_parse|global_qualified_template_member_fn_ptr_const_parse|dependent_template_member_ptr_parse|template_member_fn_ptr_const_parse|template_member_fn_ptr_ref_qual_parse)' --output-on-failure`.
- Registered the new regression under `CPP_POSITIVE_PARSE_STEMS` so the
  internal harness continues treating the file as a parse-only Step 3 probe.
- Full-suite validation stayed monotonic under the repo regression guard:
  `test_fail_before.log` 2868/2868 passed,
  `test_fail_after.log` 2869/2869 passed, with zero new failures.

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
  `ideas/open/45_inherited_record_layout_and_base_member_access_followup.md`
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
- The namespace-qualified dependent owner reduction initially exposed a
  parser-side gap rather than a pure HIR bug: `typename ns::Holder<T>::AliasL`
  / `AliasR` parsed, but dependent `typename` resolution did not fold the
  namespace prefix into owner lookup, so the cast node kept an unresolved alias
  instead of the underlying reference-qualified target. That left HIR and
  overload selection treating the cast expression like a non-lvalue reference
  alias until the qualified owner lookup was repaired.
- The global-qualified nested dependent owner reduction exposed the next parser
  gap in the same area: once the owner walk reached `::ns::Holder<T>`, nested
  record members still compared against canonicalized `ns::Inner` declarations
  instead of the source-spelled `Inner` segment, so
  `typename ::ns::Holder<T>::Inner::AliasL` / `AliasR` stayed unresolved and
  lowered through the bad by-value temporary path until the nested-owner match
  started honoring the unqualified member spelling.
- The first draft of the forwarding-reference regression also checked the
  `T=int` specialization in the same file. That exposed a separate function
  template specialization identity collision: both `forward_pick<T=int&>` and
  `forward_pick<T=int>` lowered to the same emitted function name and only one
  specialization survived in the module. That broader template-instantiation
  issue was spun out into
  `ideas/open/50_function_template_specialization_identity_collision_followup.md`
  instead of stretching this cast-focused Step 4 slice.
