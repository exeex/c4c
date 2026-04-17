# Remaining C++ Positive Cast And Specialization Member Fixes

## Goal

Fix the two remaining C++ positive-case regressions left after the
ref-overload direct-call binding repair:

- `cpp_positive_sema_c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse_cpp`
- `cpp_positive_sema_template_struct_specialization_runtime_cpp`

## Why This Is Separate

These failures are not part of the ref-qualified member-call link-name bug.

They break on different mechanisms:

- function-pointer cast/reference handling in local-declaration lowering
- specialized struct member owner/type recovery for member expressions

Keeping them in one follow-up idea is still reasonable because both are
remaining bounded correctness fixes for the same failing positive-case batch,
and the user explicitly requested continuing with items 2 and 3 together.

## Scope

- local declaration lowering for casted function-pointer/reference initializers
- member-expression owner/type recovery for realized template struct
  specializations
- focused runtime validation for the two affected tests

## Non-Goals

- revisiting the already-fixed ref-overload direct-call binding slice
- weakening tests or reclassifying runtime-positive cases
- broad parser or template-system redesigns unless a narrow fix proves
  impossible

## Acceptance Criteria

- [ ] the function-pointer cast case no longer lowers `(T (*)(...))raw` into an
      address-of-local artifact like `&raw`
- [ ] specialized struct member expressions retain the realized owner/type
      instead of dropping back to the primary-template field contract
- [ ] both remaining targeted runtime tests pass

## Validation

- build the compiler
- run the focused two-test subset:
  `cpp_positive_sema_c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse_cpp|cpp_positive_sema_template_struct_specialization_runtime_cpp`
