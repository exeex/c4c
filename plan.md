# RV64 `va_start` Helper Lowering Runbook

Status: Active
Source Idea: ideas/open/364_rv64_va_start_helper_lowering.md

## Purpose

Continue the RV64 variadic backend work after idea 362 by lowering prepared
`va_start` helper calls, starting from the current
`unsupported_variadic_helper_lowering` boundary observed in `src/920908-1.c`.

## Goal

Consume explicit prepared `va_start` helper facts in RV64 object lowering, or
emit precise target diagnostics for helper classes whose ABI homes or runtime
state are still not supportable.

## Core Rule

RV64 object emission must lower `va_start` from prepared helper facts only. Do
not infer helper semantics from source text, testcase names, LIR shape, or BIR
shape inside the target route.

## Read First

- `ideas/open/364_rv64_va_start_helper_lowering.md`
- `ideas/closed/362_rv64_scalar_vararg_and_variadic_lowering.md`
- `ideas/closed/361_rv64_vararg_abi_hook_materialization.md`
- `ideas/closed/360_lir_bir_vararg_va_arg_contract_completion.md`
- representative log for `src/920908-1.c`, if present:
  `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`
- focused backend tests around RV64 object emission, prepared vararg facts, and
  variadic helper diagnostics

## Current Scope

- prepared `va_start` helper resources, homes, and target-boundary facts
- the RV64 object lowering contract for the first supportable `va_start` helper
  path
- precise unsupported diagnostics for helper classes whose homes or runtime
  state remain incomplete
- focused backend tests for supported and unsupported `va_start` helper
  lowering
- representative RV64 allowlist proof for `src/920908-1.c`

## Non-Goals

- Do not reopen scalar `va_arg`, `va_copy`, or the idea 362 variadic admission
  milestone unless a real regression reintroduces those exact failures.
- Do not implement external variadic call lowering such as
  `PreparedCallWrapperKind::DirectExternVariadic`.
- Do not broaden into general RV64 parameter-home expansion; that belongs to
  `ideas/open/363_rv64_object_parameter_home_coverage.md`.
- Do not implement testcase-name or source-pattern shortcuts for
  `src/920908-1.c`.
- Do not improve scan results through unsupported expectation downgrades, skip
  broadening, or allowlist filtering.

## Working Model

Idea 362 narrowed the old broad RV64 variadic admission failure to a prepared
helper-specific boundary. The next repair should inspect the prepared helper
payload reaching RV64 object emission, define the first safe `va_start`
materialization path, and leave unsupported helper classes with precise target
ABI diagnostics.

## Execution Rules

- Keep implementation packets narrow enough for build proof plus focused
  backend tests.
- Treat `src/920908-1.c` as a representative signal, not an implementation
  key.
- Prefer precise unsupported diagnostics over guessed ABI homes or hidden
  runtime-state assumptions.
- Preserve existing scalar `va_arg`, `va_copy`, non-variadic RV64 object-route,
  and prepared-printer behavior.
- Escalate validation when changes affect shared prepared helper facts or
  general RV64 object admission.

## Steps

### Step 1. Audit Prepared `va_start` Helper Boundary

Goal: identify the exact prepared helper facts, RV64 target sites, homes, and
runtime-state requirements behind the current
`unsupported_variadic_helper_lowering` failure.

Concrete actions:

- Inspect the representative `src/920908-1.c` log and focused helper tests that
  reach `unsupported_variadic_helper_lowering`.
- Locate the prepared helper producer, printer/debug output, and RV64 object
  consumer for `va_start`.
- Map helper operands, resource homes, `va_list` homes, and required runtime
  state to the RV64 lowering surface that must materialize or reject them.
- Record the first implementation boundary and narrow proof command in
  `todo.md`.

Completion check:

- The next executor can name the exact RV64 target helper site, the prepared
  facts it consumes, the first supportable or unsupported helper class, and a
  narrow proof command.

### Step 2. Define the First RV64 `va_start` Lowering Contract

Goal: make the target-side contract explicit before materializing helper code.

Concrete actions:

- Determine which prepared `va_start` helper path has explicit ABI homes and
  runtime-state requirements.
- Define how RV64 object emission should initialize the destination `va_list`
  for that path.
- Identify unsupported helper classes by missing home, missing resource, or
  unsupported runtime-state requirement.
- Add or update focused tests that assert the contract through prepared facts
  and diagnostics, not source-shape matching.

Completion check:

- Focused tests distinguish the first supportable `va_start` helper contract
  from precise unsupported helper classes.

### Step 3. Materialize or Precisely Diagnose `va_start`

Goal: lower the first supportable prepared `va_start` helper path in RV64
object emission, while preserving precise diagnostics for unsupported helper
classes.

Concrete actions:

- Implement RV64 object lowering for the selected prepared `va_start` helper
  path only where homes and runtime state are explicit.
- Emit structured unsupported diagnostics when a helper path lacks a supportable
  destination home, source resource, or runtime-state requirement.
- Add focused backend tests for the materialized path and the unsupported
  diagnostics.
- Confirm the work does not conflate helper lowering with
  `PreparedCallWrapperKind::DirectExternVariadic`.

Completion check:

- Focused backend tests show prepared `va_start` helper facts are consumed by
  RV64 lowering or rejected with precise diagnostics, and the old
  `unsupported_variadic_helper_lowering` boundary is not merely renamed.

### Step 4. Representative and Baseline Validation

Goal: prove the focused helper-lowering work is stable enough to hand back to
the broader RV64 object-route backlog.

Concrete actions:

- Run build proof and focused backend tests touched by this plan.
- Run the representative RV64 allowlist case:
  - `src/920908-1.c`
- Document the representative outcome in `todo.md`.
- Create a separate open idea only if remaining work is outside this focused
  prepared `va_start` helper-lowering scope.

Completion check:

- Focused backend tests remain green, `src/920908-1.c` is improved or more
  precisely classified, and no accepted progress relies on testcase-name
  matching, expectation downgrades, or scan filtering.
