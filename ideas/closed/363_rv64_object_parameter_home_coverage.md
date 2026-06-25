# RV64 Object Parameter Home Coverage

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/closed/362_rv64_scalar_vararg_and_variadic_lowering.md`

## Goal

Extend RV64 object-route parameter-home support beyond the currently supported
GPR-only boundary so representative cases such as `src/20030914-2.c` no longer
stop at `unsupported_param_home`.

## Why This Exists

Idea 362 closed the focused scalar vararg and variadic admission milestone.
Its representative Step 5 scan showed `src/20030914-2.c` still failing with:

`unsupported_param_home: RV64 object route requires all parameters in supported GPR homes`

That residual is a general RV64 object parameter-home coverage issue, not a
scalar vararg or prepared variadic-helper fact issue.

## In Scope

- Audit the parameter homes encountered by `src/20030914-2.c` and nearby RV64
  object-route cases.
- Identify which unsupported parameter-home classes are safe to materialize
  under the existing RV64 ABI model.
- Implement semantic RV64 object lowering for the first supportable
  parameter-home class, or replace the diagnostic with a more precise ABI
  boundary when support is intentionally absent.
- Add focused backend tests that exercise the supported or precisely diagnosed
  parameter-home classes without relying on testcase names.
- Re-run `src/20030914-2.c` as a representative signal after the focused
  parameter-home work.

## Out of Scope

- Scalar `va_arg`, `va_copy`, and variadic helper lowering already covered by
  idea 362.
- Broad vector ABI, FPR ABI, globals, or unrelated instruction-selection work
  unless the audited parameter-home class directly requires it.
- Rewriting RV64 object admission around a specific torture testcase shape.
- Improving scan counts through unsupported expectation downgrades, skip
  broadening, or allowlist filtering.

## Acceptance Criteria

- Focused backend tests prove the selected RV64 parameter-home class is
  materialized correctly or rejected with a more precise unsupported diagnostic.
- `src/20030914-2.c` is rerun and its outcome is documented.
- Existing focused backend coverage for RV64 object emission remains green.
- Progress is based on structured ABI homes, not testcase-name or source-shape
  matching.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c` or matching its exact
  source shape instead of lowering a real parameter-home class.
- Reject unsupported expectation downgrades, skip broadening, or allowlist
  filtering claimed as parameter-home progress.
- Reject diagnostic-only renames that leave the same unsupported parameter-home
  class unexplained behind new wording.
- Reject broad RV64 object-route rewrites that do not directly materialize or
  precisely diagnose the audited parameter-home class.
- Reject changes that regress GPR parameter-home behavior while adding a new
  unsupported class.

## Starting Signal

Representative scan from idea 362:

- `src/20030914-2.c` fails at `unsupported_param_home`
- case log: `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`

## Closure Note

Closed after focused RV64 object-emission coverage proved the audited
parameter-home class with a precise diagnostic:

```text
unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots
```

The focused backend subset stayed green before and after closure validation:
`backend_riscv_object_emission`,
`backend_prepare_frame_stack_call_contract`, and
`backend_prepared_printer` all passed.

The representative `src/20030914-2.c` was rerun. It no longer provides a
direct parameter-home proof in the current route because it stops earlier at:

```text
unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract
```

That earlier admission boundary is out of scope for this parameter-home idea
and is tracked separately in
`ideas/open/367_rv64_helper_free_variadic_entry_contract.md`.
