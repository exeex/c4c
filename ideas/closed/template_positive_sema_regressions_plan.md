# Positive Template/Sema Regression Fix Plan

Status: Complete
Completed On: 2026-03-28

Target area: `src/frontend/hir/ast_to_hir.cpp` and adjacent frontend template/sema parsing/lowering code  
Primary goal: fix the current positive C++ semantic/template regressions without broad unrelated refactors.

## Why This Idea

The current suite still has a small cluster of persistent positive-case frontend
failures centered around template argument handling, substitution, and
expression parsing. These are valuable because they represent real supported
language shapes that should pass without requiring runtime workarounds.

Current failing targets:
- `222 - cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
- `224 - cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
- `233 - cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
- `396 - cpp_positive_sema_template_arg_deduction_cpp`
- `416 - cpp_positive_sema_template_mixed_params_cpp`
- `437 - cpp_positive_sema_template_type_subst_cpp`

## Objective

Drive the above failures to passing by fixing the minimum necessary frontend
template/sema behavior, with focused regression coverage and no accidental
expansion into unrelated parser or codegen cleanups.

## Suspected Hot Regions

Likely implementation surfaces:
- `src/frontend/hir/ast_to_hir.cpp`
- template argument parsing and substitution helpers
- deferred expression parsing/evaluation used by template-dependent contexts
- call/lvalue classification around instantiated call results

Likely themes:
- pack expansion parsing inside template argument lists
- template argument deduction across mixed type/non-type parameter sets
- type substitution through dependent aliases/helpers
- dependent base-expression parsing used by EASTL/type-traits probes
- preserving lvalue category for call results when sema expects a bindable
  reference/object result

## Execution Plan

### 1. Capture precise failure signatures

For each failing testcase:
- record the first frontend error or mismatch
- group failures by shared mechanism instead of fixing in numeric order
- note whether the break is parse, substitution, deduction, or value-category
  related

### 2. Fix the narrowest common mechanism first

Preferred order:
- pack/template-argument parse bugs
- deduction/substitution bugs shared by multiple tests
- lvalue/call-result classification bugs
- EASTL-specific dependent expression handling if still separate

Rule:
- prefer one mechanism family per patch rather than six ad hoc testcase hacks

### 3. Add focused regression coverage

Before or alongside each fix:
- add the narrowest internal regression that isolates the mechanism
- keep the original positive testcase as end-to-end confirmation
- avoid relying only on large EASTL probes when a smaller HIR/frontend case can
  pin the intended behavior

### 4. Validate with monotonic regression guard

Validation target:
- the six listed positive cases pass
- no newly failing tests appear in the full suite
- if one fix uncovers a separate unsupported feature, spin that out into a new
  idea instead of silently growing scope

## Guardrails

- Do not fold in broad `ast_to_hir.cpp` cleanup while chasing these regressions.
- Do not special-case EASTL text if the underlying bug is generic template/sema
  handling.
- Keep parser, deduction, and substitution fixes behavior-driven and covered by
  focused tests.
- If one failure requires a new language feature rather than a bug fix, split
  it into its own idea.

## Success Criteria

- All six listed tests pass:
  - `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  - `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
  - `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
  - `cpp_positive_sema_template_arg_deduction_cpp`
  - `cpp_positive_sema_template_mixed_params_cpp`
  - `cpp_positive_sema_template_type_subst_cpp`
- Full-suite failure count does not regress.
- Any leftover unsupported feature discovered during this work is documented as
  a separate open idea.

## Completion Notes

- Completed the planned regression slices with focused coverage for pack
  expansion parsing, reference-returning call lvalues, inherited temporary
  `operator()` dispatch, and template pointer-parameter substitution.
- Verified on 2026-03-28 that all six named success-criteria tests pass in the
  rebuilt suite.
- Verified monotonic full-suite behavior with the regression guard:
  `2239 passed / 1 failed / 2240 total` before and after the final clean
  rebuild validation.

## Leftover Issues

- `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp` still fails in
  the full suite with `C2LL_RUNTIME_UNEXPECTED_RETURN`, but it was outside this
  plan's scoped success criteria and remains for separate follow-up.
