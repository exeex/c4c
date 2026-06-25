# RV64 `va_start` Helper Lowering

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/closed/362_rv64_scalar_vararg_and_variadic_lowering.md`

## Goal

Lower prepared RV64 variadic helper calls for `va_start`, starting from the
current `unsupported_variadic_helper_lowering` boundary seen in
`src/920908-1.c`.

## Why This Exists

Idea 362 replaced the broad RV64 object variadic admission failure with a
narrower prepared-helper-specific boundary. Step 5 representative validation
showed `src/920908-1.c` still failing with:

`unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_start helper`

That residual is follow-up helper-lowering work. It should prove real
materialization of prepared `va_start` helper facts, not another diagnostic
rename.

## In Scope

- Audit the prepared `va_start` helper resources, homes, and RV64 target
  boundary used by the representative failure.
- Define the RV64 object lowering contract for the first supportable
  `va_start` helper path.
- Materialize that helper path when ABI homes and runtime-state requirements
  are explicit and unambiguous.
- Emit precise unsupported diagnostics for helper classes whose homes or
  runtime-state requirements are still missing.
- Add focused backend tests for supported and unsupported prepared `va_start`
  helper lowering.
- Re-run `src/920908-1.c` as a representative signal after the focused helper
  work.

## Out of Scope

- Reopening scalar `va_arg`, `va_copy`, or the idea 362 admission-gate
  milestone unless a real regression reintroduces those exact failures.
- External variadic call lowering such as
  `PreparedCallWrapperKind::DirectExternVariadic`.
- General RV64 parameter-home expansion, tracked separately in
  `ideas/open/363_rv64_object_parameter_home_coverage.md`.
- Testcase-name matching, source-pattern shortcuts, or expectation downgrades.

## Acceptance Criteria

- Focused backend tests show prepared `va_start` helper facts are consumed by
  RV64 object lowering or rejected with precise target diagnostics.
- The old broad variadic admission failure and the current
  `unsupported_variadic_helper_lowering` boundary are not merely renamed while
  retaining the same missing helper materialization.
- `src/920908-1.c` is rerun and its outcome is documented.
- Existing focused backend coverage for RV64 object emission and prepared
  vararg facts remains green.

## Closure Note

Closed after Step 4 validation. RV64 object emission now has a focused
prepared `va_start` materialization path when the overflow-area initial base
state is explicit, and unsupported helper classes produce precise target
diagnostics instead of the old generic unlowered-helper boundary.

The representative `src/920908-1.c` was rerun and remains failing at:

`unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`

That residual is outside this object-emission helper-lowering milestone and is
tracked separately as prepared overflow-area base-state production/runtime
variadic-entry state materialization work.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/920908-1.c` or matching its exact
  source shape instead of lowering prepared `va_start` helper facts.
- Reject helper renames, expectation rewrites, or classification-only changes
  claimed as `va_start` helper-lowering progress.
- Reject lowering that infers variadic semantics from source text, testcase
  names, LIR shape, or BIR shape instead of consuming prepared helper facts.
- Reject conflating `va_start` helper lowering with external variadic call
  support such as `PreparedCallWrapperKind::DirectExternVariadic`.
- Reject broad RV64 object-route rewrites that do not directly materialize or
  precisely diagnose the prepared `va_start` helper path.

## Starting Signal

Representative scan from idea 362:

- `src/920908-1.c` fails at `unsupported_variadic_helper_lowering` for
  `va_start`
- case log: `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`
