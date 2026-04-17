# Remaining C++ Positive Cast And Specialization Member Fixes

Status: Closed
Last Updated: 2026-04-17

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

- [x] the function-pointer cast case no longer lowers `(T (*)(...))raw` into an
      address-of-local artifact like `&raw`
- [x] specialized struct member expressions retain the realized owner/type
      instead of dropping back to the primary-template field contract
- [x] both remaining targeted runtime tests pass

## Validation

- build the compiler
- run the focused two-test subset:
  `cpp_positive_sema_c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse_cpp|cpp_positive_sema_template_struct_specialization_runtime_cpp`

## Completion Notes

- Completed in commit `814a3729` on 2026-04-17.
- Fixed function-pointer cast local-init lowering so callable casts stay value
  carriers instead of collapsing into operand addresses or fake reference
  storage.
- Fixed realized template-specialization member owner/type recovery so member
  expressions keep the specialized field contract instead of falling back to
  stale primary-template AST state.
- Focused validation passed for the two targeted regressions, the original
  eight-case failure batch, and broader frontend checks:
  `frontend_hir_tests` and `frontend_parser_tests`.
