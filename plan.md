# RV64 `va_start` Destination `va_list` GPR Home Runbook

Status: Active
Source Idea: ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md

## Purpose

Continue the RV64 `va_start` work after idea 365 by making the destination
`va_list` address state explicit enough for helper-store lowering.

## Goal

Publish or prove the prepared destination `va_list` address in a GPR home
before RV64 object emission consumes `va_start` helper facts.

## Core Rule

RV64 `va_start` helper lowering may store initialized `va_list` fields only
when the destination `va_list` address is represented by an explicit prepared
GPR-home contract. Do not synthesize the address inside object emission by
guessing source shape, frame layout, or testcase identity.

## Read First

- `ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md`
- `ideas/closed/365_rv64_va_start_overflow_base_state_production.md`
- `ideas/closed/364_rv64_va_start_helper_lowering.md`
- focused tests around prepared variadic helper facts, prepared printer output,
  RV64 object emission, prepared frame stack call contracts, and
  variadic-entry diagnostics
- representative log for `src/920908-1.c`, if present:
  `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`

## Current Scope

- the prepared fact or producer boundary for the destination `va_list` address
  used by RV64 `va_start` helper stores
- the definition of when that destination address is resident in, or
  materializable from, a prepared GPR home
- precise unsupported diagnostics when the prepared address contract is absent
- focused backend or prepared-contract tests for supported destination-address
  materialization and the unsupported boundary
- representative RV64 allowlist proof for `src/920908-1.c`

## Non-Goals

- Do not rework overflow-area base-state production from idea 365.
- Do not reopen the idea-364 RV64 object-emission helper-store path except for
  narrow consumer adjustments required by the finalized destination-address
  contract.
- Do not broaden into general RV64 parameter-home expansion; that belongs to
  `ideas/open/363_rv64_object_parameter_home_coverage.md`.
- Do not implement external variadic call lowering such as
  `PreparedCallWrapperKind::DirectExternVariadic`.
- Do not implement scalar `va_arg`, `va_copy`, aggregate `va_arg`, or broad
  variadic ABI rewrites beyond the destination `va_list` address contract.
- Do not improve scan results through testcase-name matching, source-pattern
  shortcuts, unsupported expectation downgrades, allowlist filtering, or
  scan-only classification changes.

## Working Model

Idea 365 completed the overflow-area initial base-state producer. The
representative `src/920908-1.c` case now advances to this later boundary:

```text
unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home
```

This plan should determine where the prepared pipeline already knows, or can
legitimately publish, the address of the destination `va_list` object for a
supportable RV64 `va_start`. RV64 object emission should consume that prepared
contract, not reconstruct the address from unrelated runtime state.

## Execution Rules

- Keep implementation packets narrow enough for build proof plus focused
  prepared-contract or backend tests.
- Treat `src/920908-1.c` as a representative signal, not an implementation
  key.
- Prefer precise unsupported diagnostics over implicit address reconstruction.
- Preserve existing RV64 `va_start` helper-store behavior where destination
  address state and overflow-area base state are already explicit.
- Escalate validation when changes affect shared prepared-module facts,
  variadic-entry lowering, value-home publication, or general RV64 object
  admission.

## Steps

### Step 1. Audit Destination Address Producer Boundary

Goal: identify the exact producer or contract gap behind the missing prepared
GPR-home destination-address diagnostic.

Concrete actions:

- Inspect the representative `src/920908-1.c` log and focused tests that reach
  the destination `va_list` prepared-GPR-home diagnostic.
- Locate the prepared variadic-helper fact that names the destination
  `va_list`, the value-home or GPR-home facts available for that address, and
  the RV64 object consumer that currently rejects it.
- Determine whether the missing state is a producer omission, a value-home
  publication gap, or a consumer contract mismatch.
- Record the first implementation boundary and narrow proof command in
  `todo.md`.

Completion check:

- The next executor can name the producer site, the RV64 consumer check, the
  current missing fact path, and a narrow proof command before changing code.

### Step 2. Define the Prepared Destination-Address Contract

Goal: make the supported destination `va_list` address representation explicit
before helper stores consume it.

Concrete actions:

- Define when a `va_start` destination address is considered available in a
  prepared GPR home.
- Specify how that address relates to prepared value homes, frame slots,
  local-memory homes, and helper-store operands.
- Decide which missing-address cases should remain unsupported and what
  diagnostic evidence they should carry.
- Add or update focused tests that assert the contract through prepared facts,
  printer output, backend diagnostics, or object-emission behavior without
  relying on source-shape matching.

Completion check:

- Focused tests distinguish supported destination-address materialization from
  entries that must still be precisely rejected.

### Step 3. Publish or Materialize the Destination `va_list` Address

Goal: make supportable RV64 `va_start` helper entries expose the destination
address in the prepared GPR-home form required by object emission.

Concrete actions:

- Implement the selected producer or materialization path for the destination
  `va_list` address.
- Keep RV64 object helper-store lowering gated on explicit prepared address
  state and existing overflow-area base-state facts.
- Preserve or sharpen unsupported diagnostics when the address contract cannot
  be satisfied.
- Add focused backend or prepared-contract tests for both the produced address
  state and the unsupported boundary.

Completion check:

- Focused tests show the prepared destination address is available for the
  supportable path, and helper lowering remains rejected when the explicit
  GPR-home address contract is absent.

### Step 4. Representative and Baseline Validation

Goal: prove the destination-address contract is stable enough to hand back to
the broader RV64 object-route backlog.

Concrete actions:

- Run build proof and focused tests touched by this plan.
- Run the representative RV64 allowlist case:
  - `src/920908-1.c`
- Document the representative outcome in `todo.md`.
- Create a separate open idea only if remaining work is outside this focused
  destination-address scope.

Completion check:

- Existing focused RV64 object emission, prepared frame stack call contract,
  prepared printer output, and variadic-entry diagnostic coverage remains
  green; `src/920908-1.c` is rerun and its outcome is documented as real
  destination-address progress rather than a renamed diagnostic.
