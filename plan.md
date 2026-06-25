# RV64 `va_start` Overflow Base-State Production Runbook

Status: Active
Source Idea: ideas/open/365_rv64_va_start_overflow_base_state_production.md

## Purpose

Continue the RV64 `va_start` work after idea 364 by producing explicit
prepared overflow-area initial base state for variadic function entries.

## Goal

Publish the base slot and signed stack offset that RV64 prepared `va_start`
lowering needs to initialize the `va_list` overflow field without guessing
target runtime state.

## Core Rule

Prepared `va_start` lowering may consume overflow-area base state only when a
producer has materialized an explicit RV64 variadic-entry runtime contract. Do
not synthesize or guess the overflow base inside object emission.

## Read First

- `ideas/open/365_rv64_va_start_overflow_base_state_production.md`
- `ideas/closed/364_rv64_va_start_helper_lowering.md`
- focused tests around prepared variadic helper facts, prepared printer output,
  RV64 object emission, and variadic-entry diagnostics
- representative log for `src/920908-1.c`, if present:
  `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`

## Current Scope

- the RV64 prepared-module contract for overflow-area initial base state at
  variadic function entry
- the producer that can publish `overflow_area.base_slot_id` and
  `base_stack_offset_bytes`
- helper-free RV64 variadic entry state only when the runtime contract is
  explicit and supportable
- focused backend or prepared-contract tests proving the producer facts before
  object emission consumes them
- representative RV64 allowlist proof for `src/920908-1.c`

## Non-Goals

- Do not rework the idea-364 RV64 object-emission `va_start` store path except
  for narrow consumer adjustments required by the finalized producer contract.
- Do not implement external variadic call lowering such as
  `PreparedCallWrapperKind::DirectExternVariadic`.
- Do not broaden into general RV64 parameter-home expansion; that belongs to
  `ideas/open/363_rv64_object_parameter_home_coverage.md`.
- Do not implement scalar `va_arg`, `va_copy`, aggregate `va_arg`, or broad
  variadic ABI rewrites beyond the overflow-area initial base-state producer.
- Do not improve scan results through testcase-name matching, source-pattern
  shortcuts, unsupported expectation downgrades, allowlist filtering, or
  scan-only classification changes.

## Working Model

Idea 364 taught RV64 object emission how to lower prepared `va_start` helper
facts when the overflow-area initial base state is already explicit. The
remaining failure for `src/920908-1.c` is now a producer/runtime-state boundary:

```text
unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state
```

This plan should find where variadic-entry runtime state is produced, define
the prepared fact contract for the overflow area, and make object emission
consume that contract without falling back to target-side guesses.

## Execution Rules

- Keep implementation packets narrow enough for build proof plus focused
  prepared-contract or backend tests.
- Treat `src/920908-1.c` as a representative signal, not an implementation
  key.
- Prefer precise unsupported diagnostics over implicit runtime-state
  assumptions.
- Preserve existing RV64 object `va_start` helper-store behavior where prepared
  overflow-area base state is already explicit.
- Escalate validation when changes affect shared prepared-module facts,
  variadic-entry lowering, or general RV64 object admission.

## Steps

### Step 1. Audit Overflow Base-State Producer Boundary

Goal: identify the exact producer gap behind the missing prepared
overflow-area initial base-state diagnostic.

Concrete actions:

- Inspect the representative `src/920908-1.c` log and focused tests that reach
  the missing-base-state diagnostic.
- Locate the prepared variadic-entry producer, prepared printer/debug output,
  and RV64 object consumer fields for overflow-area state.
- Map the required `overflow_area.base_slot_id` and
  `base_stack_offset_bytes` facts to the frame slot or stack-layout state that
  can legitimately publish them.
- Record the first implementation boundary and narrow proof command in
  `todo.md`.

Completion check:

- The next executor can name the prepared producer site, the RV64 consumer
  fields, the current missing fact path, and a narrow proof command before
  changing code.

### Step 2. Define the RV64 Prepared Overflow-Area Contract

Goal: make the producer/runtime-state contract explicit before materializing
helper-free variadic entry state.

Concrete actions:

- Define when an RV64 variadic entry has explicit overflow-area initial base
  state.
- Specify the meaning and sign convention of `base_stack_offset_bytes`.
- Specify how `overflow_area.base_slot_id` relates to prepared frame slots,
  stack layout, and the `va_list` overflow field consumed by `va_start`
  lowering.
- Add or update focused tests that assert the contract through prepared facts,
  printer output, or diagnostics without relying on source-shape matching.

Completion check:

- Focused tests distinguish supported RV64 entries that publish overflow-area
  initial base state from entries that must still be precisely rejected.

### Step 3. Materialize Overflow Base State in Prepared Facts

Goal: publish the explicit overflow-area initial base state for supportable
RV64 variadic entries.

Concrete actions:

- Implement the selected producer path so supportable RV64 variadic entries
  publish `overflow_area.base_slot_id` and `base_stack_offset_bytes`.
- Keep RV64 object `va_start` lowering gated on explicit prepared facts rather
  than target-side reconstruction.
- Emit or preserve precise unsupported diagnostics when the prepared contract
  cannot be materialized.
- Add focused backend or prepared-contract tests for both the produced facts
  and the unsupported boundary.

Completion check:

- Focused tests show the producer emits the required overflow-area base state,
  and helper-free RV64 variadic entry remains rejected unless the explicit
  runtime contract exists.

### Step 4. Representative and Baseline Validation

Goal: prove the overflow-base-state producer work is stable enough to hand
back to the broader RV64 object-route backlog.

Concrete actions:

- Run build proof and focused tests touched by this plan.
- Run the representative RV64 allowlist case:
  - `src/920908-1.c`
- Document the representative outcome in `todo.md`.
- Create a separate open idea only if remaining work is outside this focused
  overflow-area base-state production scope.

Completion check:

- Existing focused RV64 object emission, prepared frame stack call contract,
  prepared printer output, and variadic-entry diagnostic coverage remains
  green; `src/920908-1.c` is rerun and its outcome is documented as real
  producer/runtime-state progress rather than a renamed diagnostic.
