# Rvalue Reference Completeness Todo

Status: Active
Source Idea: ideas/open/46_rvalue_reference_completeness_plan.md
Source Plan: plan.md

## Active Item

- Active implementation slice completed: forwarding-reference wrappers now
  deduce call-expression prvalues such as `make()` and preserve ref-qualified
  member dispatch across both `T=Probe&` and `T=Probe` specializations.

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
- Added `template_helper_return_overload_basic.cpp` to lock explicit-template
  helper calls returning instantiated `T&` and `T&&` into downstream
  ref-overload sets.
- Resolve explicit-template call return types back to their instantiated
  callees when HIR decides whether a call expression is an lvalue or already
  uses the reference ABI, avoiding `T&` helper results being copied into
  rvalue temporaries.
- Re-ran the focused helper-return regression cluster plus the full suite;
  regression guard passed with pass count improving from 3159 to 3161 and
  zero new failures.
- Added `forwarding_ref_qualified_member_dispatch.cpp` to lock forwarding
  wrappers that route lvalue and call-expression prvalue receivers into `&`
  vs `&&` ref-qualified member overloads.
- Teach template deduction to recover plain call-expression argument types
  from early function-definition metadata so wrappers like `call_select(make())`
  seed the `T=Probe` specialization instead of staying unresolved.
- Defer early `infer_generic_ctrl_type()` returns for template-bound typedefs
  so instantiated casts such as `static_cast<T&&>(t)` substitute `T` before
  method lookup and ref-overload selection.
- Encode lvalue/rvalue reference qualifiers in template-function mangled names
  so `T=Probe&` and `T=Probe` specializations no longer collide at materialization.
- Re-ran the focused ref-qualified-member and named-rvalue-reference cluster
  plus the full suite; regression guard passed with pass count improving from
  3159 to 3162 and zero new failures.

## Next Slice

- probe ref-qualified member dispatch or binding only if a remaining generic
  `&&` defect is demonstrated beyond the direct, explicit-template, and
  forwarding-wrapper receiver paths now covered
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
- explicit-template helper calls such as `as_lvalue<int>(...)` now resolve
  their instantiated return type during HIR call lowering, so both overload
  choice and reference-ABI argument passing see `T&` vs `T&&` correctly
- call-expression arguments such as `make()` now participate in forwarding
  reference deduction early enough to materialize the correct `T=Probe`
  specialization before ordinary functions are lowered into the module
- template-bound typedef spellings like `static_cast<T&&>(t)` now substitute
  `T` before generic control-type inference short-circuits, which keeps
  method lookup on `Probe::select` on the method path instead of falling back
  to field-style member access
- template function mangling now distinguishes `T=Probe&` from `T=Probe`, so
  ref-collapsed forwarding and plain value specializations can coexist
- keep EASTL or libstdc++ findings scoped to generic language defects only
