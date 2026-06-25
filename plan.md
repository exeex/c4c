# RV64 Object Parameter Home Coverage Runbook

Status: Active
Source Idea: ideas/open/363_rv64_object_parameter_home_coverage.md

## Purpose

Continue the RV64 object-route backlog after the scalar variadic milestone by
auditing and repairing the residual parameter-home boundary reported by
`src/20030914-2.c`.

## Goal

Materialize the first supportable RV64 object parameter-home class, or replace
the broad `unsupported_param_home` failure with a precise ABI boundary backed by
focused tests.

## Core Rule

Progress must be based on structured RV64 parameter-home classes and prepared
ABI facts, not testcase names, source-shape matching, expectation downgrades,
or broader object-route rewrites.

## Read First

- `ideas/open/363_rv64_object_parameter_home_coverage.md`
- `ideas/closed/362_rv64_scalar_vararg_and_variadic_lowering.md`
- representative log, if present:
  `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`
- RV64 object-emission parameter-home admission and diagnostics
- focused backend tests for RV64 object emission, prepared value homes, and
  parameter/frame-home contracts

## Current Scope

- the parameter-home classes encountered by `src/20030914-2.c` and nearby RV64
  object-route cases
- the first parameter-home class that can be supported under the existing RV64
  ABI model
- precise unsupported diagnostics for classes that cannot yet be materialized
- focused backend coverage for supported and rejected parameter-home classes
- a representative rerun of `src/20030914-2.c`

## Non-Goals

- Do not reopen scalar `va_arg`, `va_copy`, variadic helper lowering, or
  variadic admission work already covered by idea 362 and its follow-ups.
- Do not implement broad vector ABI, unrelated FPR ABI, globals, data sections,
  or instruction-selection work unless the audited parameter-home class directly
  requires it.
- Do not replace the whole RV64 object-route architecture.
- Do not improve scan results through unsupported expectation downgrades,
  broader skips, allowlist filtering, or testcase-shaped shortcuts.

## Working Model

The current representative failure is:

```text
unsupported_param_home: RV64 object route requires all parameters in supported GPR homes
```

This plan should determine which concrete prepared home triggers that boundary,
whether that home is semantically supportable by the current RV64 object route,
and what the narrow producer/consumer contract should be. If support is not yet
valid, the result should be a more precise diagnostic with tests that identify
the unsupported ABI home class.

## Execution Rules

- Start from observed prepared parameter-home facts, not from the source text of
  `src/20030914-2.c`.
- Keep changes in narrow packets with build proof plus focused backend tests.
- Preserve existing GPR parameter-home behavior while adding or diagnosing the
  new class.
- Prefer a precise unsupported boundary over guessed lowering.
- Escalate validation if the implementation changes shared value-home,
  prepared-call, frame-home, or RV64 object-admission behavior.

## Steps

### Step 1. Audit Unsupported Parameter Homes

Goal: identify the exact parameter-home class behind the representative
`unsupported_param_home` diagnostic.

Concrete actions:

- Inspect the current `src/20030914-2.c` case log and regenerate focused dumps
  if the available log does not expose the prepared homes.
- Locate the RV64 object-route check that requires supported GPR homes.
- Record every parameter home encountered by the representative case and any
  nearby same-family case selected during the audit.
- Decide whether the first boundary is a stack home, byval aggregate, mixed GPR
  group, indirect pointer, or another structured home class.
- Record the first implementation boundary and narrow proof command in
  `todo.md`.

Completion check:

- The next executor can name the unsupported home class, the producer facts,
  the RV64 consumer check, and a narrow test command before changing code.

### Step 2. Define the Parameter-Home Contract

Goal: define the supported or precisely rejected RV64 ABI contract for the
audited parameter-home class.

Concrete actions:

- Specify which prepared facts are required to lower the audited home class.
- Decide whether the class can be emitted under the current object route or
  must remain unsupported.
- Add focused tests that assert the supported contract or the precise rejected
  diagnostic without relying on `src/20030914-2.c`.
- Keep existing GPR-home tests green and explicit.

Completion check:

- Focused tests distinguish ordinary supported GPR homes from the audited
  parameter-home class and document the expected behavior.

### Step 3. Materialize or Precisely Diagnose the Home Class

Goal: implement the selected support path, or replace the broad diagnostic with
  a precise structured unsupported boundary.

Concrete actions:

- Implement semantic RV64 lowering only if the prepared facts are sufficient.
- If support is not valid yet, emit a specific diagnostic for the audited home
  class and preserve the broader unsupported boundary for other unknown homes.
- Add or update focused backend tests for the implementation or diagnostic.
- Preserve existing behavior for already supported GPR parameter homes.

Completion check:

- Focused backend tests prove the new support or diagnostic, and no existing
  GPR parameter-home coverage regresses.

### Step 4. Representative and Baseline Validation

Goal: prove the parameter-home work against the representative case and record
the next boundary honestly.

Concrete actions:

- Run the focused backend test subset touched by this plan.
- Run the representative allowlist case:
  - `src/20030914-2.c`
- Document whether the case executes, advances to a later structured boundary,
  or remains blocked by an out-of-scope home class.
- Create a separate open idea only if remaining work is outside this focused
  parameter-home scope.

Completion check:

- Focused RV64 object-emission coverage remains green; `src/20030914-2.c` is
  rerun and its outcome is documented as structured parameter-home progress
  rather than a renamed failure.
