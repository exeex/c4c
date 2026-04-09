# Rvalue Reference Completeness Todo

Status: Active
Source Idea: ideas/open/46_rvalue_reference_completeness_plan.md
Source Plan: plan.md

## Active Item

- Step 1 audit outcome: existing parser coverage already covers top-level
  deduction guides, template-id conversion operators, and generated
  disambiguation matrix cases for `&&`.
- Active implementation slice completed: `const T&&` now rejects lvalue
  arguments during template deduction instead of being treated as a forwarding
  reference.
- Active implementation slice completed: local `auto&&` deduction from lvalue
  initializers now collapses to lvalue-reference storage in sema and HIR
  instead of tripping the plain rvalue-reference binding rule or materializing
  a temporary.

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

## Next Slice

- add focused coverage for named rvalue-reference variables remaining lvalues
  in overload resolution and forwarding paths
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
- keep EASTL or libstdc++ findings scoped to generic language defects only
