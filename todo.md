# Rvalue Reference Completeness Todo

Status: Active
Source Idea: ideas/open/46_rvalue_reference_completeness_plan.md
Source Plan: plan.md

## Active Item

- Step 1 audit outcome: existing parser coverage already covers top-level
  deduction guides, template-id conversion operators, and generated
  disambiguation matrix cases for `&&`.
- Active implementation slice completed: helper-return paths for a named
  `T&&` parameter now preserve `T&` lvalue behavior for downstream overload
  selection while explicit forwarding back to `T&&` still selects the rvalue
  overload.

## Completed

- Activated `plan.md` from `ideas/open/46_rvalue_reference_completeness_plan.md`.
- Audited current `&&` coverage enough to classify the first concrete gap:
  parser coverage is broader than sema coverage, and `const T&&` currently
  accepts lvalue arguments through incorrect template deduction.
- Added `bad_const_template_rvalue_ref_bind_lvalue.cpp` to lock the
  `const T&&` rule with a focused negative regression.
- Restricted forwarding-reference deduction to unqualified `T&&` and reject
  lvalue-driven template calls that fail this rule before lowering.
- Re-ran targeted `&&` tests plus the full suite; regression guard passed with
  pass count improving from 3155 to 3156 and zero new failures.
- Added `auto_rvalue_ref_lvalue_init_basic.cpp` to lock local `auto&&`
  lvalue-initializer deduction, aliasing, and overload behavior.
- Deduce local `auto&&` declarations from initializer value category in sema
  and HIR so lvalue initializers bind through reference storage instead of
  being rejected or copied into rvalue-reference temporaries.
- Re-ran the focused `&&` regression set plus the full suite; regression guard
  passed with pass count improving from 3155 to 3157 and zero new failures.
- Added `named_rvalue_ref_lvalue_overload_basic.cpp` to lock the named
  rvalue-reference lvalue rule for plain overload resolution, explicit
  `static_cast<T&&>`, and forwarding-helper recovery.
- Re-ran the focused named-rvalue-reference regression cluster plus the full
  suite; regression guard passed with pass count improving from 3155 to 3158
  and zero new failures.
- Added `bad_named_rvalue_ref_return_lvalue.cpp` to lock the rule that named
  `T&&` parameters stay lvalues in `return` statements unless explicitly cast
  or forwarded.
- Reject lvalue returns from `T&&` functions in sema by applying reference
  binding checks before return-type compatibility.
- Re-ran the focused named-rvalue-reference return and overload tests plus the
  full suite; regression guard passed with pass count improving from 3155 to
  3159 and zero new failures.
- Added `named_rvalue_ref_helper_return_overload_basic.cpp` to lock helper
  return paths where a named `T&&` parameter is returned as `T&` or forwarded
  back as `T&&` before downstream overload resolution.
- Restore lvalue call-result tracking in sema and keep reference-returning
  helper calls on the reference ABI path in HIR so overload selection matches
  the helper return type.
- Re-ran the focused named-rvalue-reference helper-return cluster plus the
  full suite; regression guard passed with pass count improving from 3159 to
  3160 and zero new failures.

## Next Slice

- probe template-instantiated helper returns only if a remaining generic `&&`
  defect is demonstrated beyond the direct function-return path now covered
- keep any further call-category follow-up bounded to concrete overload or
  binding mismatches against Clang
- keep adjacent library-facing probes bounded to generic language defects only

## Blockers

- none recorded

## Resume Notes

- Clang rejects `probe(x)` for `template <class T> int probe(const T&&)` with
-  `x` an lvalue; `c4cll` now matches that behavior via targeted deduction
  rejection plus a lowering-time guard for explicitly rejected template calls
- keep an eye on nearby deduction helpers like function-name and `auto&&`
  inference so future `&&` fixes do not fall back to unresolved template calls
- local `auto&& ref = x;` now aliases the original lvalue in lowered IR, so
  the next bounded slice can stay focused on named-rvalue-reference lvalue
  behavior in overload and forwarding paths
- named `int&& ref` already lowers through lvalue overload selection, while
  explicit `static_cast<int&&>(ref)` and `forward_value<int>(ref)` still route
  to the rvalue overload as expected
- return validation now strips reference wrappers for type compatibility but
  separately enforces lvalue/rvalue binding rules, matching the existing
  declaration-initialization behavior more closely
- direct call expressions returning `T&` now restore lvalue behavior in sema
  and HIR ref-overload selection, while reference-returning helper calls stay
  on the reference ABI path instead of materializing temporary by-value args
- keep EASTL or libstdc++ findings scoped to generic language defects only
