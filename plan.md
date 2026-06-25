# RV64 Vararg ABI Hook Materialization Runbook

Status: Active
Source Idea: ideas/open/361_rv64_vararg_abi_hook_materialization.md

## Purpose

Consume the shared vararg contract from idea 360 in RV64 target ABI code and
materialize the explicit hook facts needed by representative variadic cases.

## Goal

Implement narrow RV64 target ABI hook/materialization support for variadic
entry state, `va_list` layout, helper resources, and aggregate `va_arg`
operand homes, or report more specific target ABI diagnostics where support is
still intentionally missing.

## Core Rule

RV64 target code may only consume prepared vararg facts emitted by the shared
contract. Do not infer vararg semantics from source, testcase names, LIR, or
BIR shape inside object emission.

## Read First

- `ideas/open/361_rv64_vararg_abi_hook_materialization.md`
- `ideas/closed/360_lir_bir_vararg_va_arg_contract_completion.md`
- RV64 object-admission diagnostics for `target_abi.*`,
  `helper_resources.*`, and `helper_operand_homes.*`
- representative case logs for `src/20030914-2.c` and `src/920908-1.c`

## Current Scope

- RV64 variadic entry state and `va_list` layout.
- RV64 helper resources for `va_start` and aggregate `va_arg`.
- Backend/prepared tests that prove hook publication, consumption, and precise
  unsupported diagnostics.
- Representative RV64 allowlist proof for `src/20030914-2.c` and
  `src/920908-1.c`.

## Non-Goals

- Do not change the shared vararg contract unless a real upstream defect is
  found and routed to a separate idea.
- Do not improve torture scan counts through skips, expectation downgrades, or
  testcase filtering.
- Do not broaden into RV64 globals, vector ABI, unrelated FPR ABI, or generic
  instruction coverage.
- Do not hide unsupported aggregate varargs behind generic prepared-object
  rejection.

## Working Model

Idea 360 made the pipeline publish explicit prepared vararg facts. This plan
starts at the RV64 target boundary and turns those facts into either concrete
ABI materialization or precise target-level diagnostics.

## Execution Rules

- Keep each packet narrow enough for build proof plus focused backend tests.
- Prefer adding a structured diagnostic over guessing an ABI location.
- Preserve non-vararg RV64 object-route behavior.
- Treat `src/20030914-2.c` and `src/920908-1.c` as representatives, not
  implementation keys.
- Escalate validation after hook changes affect shared RV64 admission paths.

## Steps

### Step 1. Audit RV64 Vararg Target Requirements

Goal: map each current object-admission missing fact to the RV64 target code
that should materialize or diagnose it.

Concrete actions:

- Reproduce or inspect the current representative logs for `src/20030914-2.c`
  and `src/920908-1.c`.
- Identify the RV64 admission/hook surfaces that consume
  `target_abi.variadic_entry_state`, `target_abi.va_list_layout`,
  `helper_resources.*`, and `helper_operand_homes.*`.
- Record the first implementation boundary and any tests that already cover it
  in `todo.md`.

Completion check:

- The next executor can name the exact RV64 target hook or admission site for
  each missing fact and the narrow proof command for the first implementation
  packet.

### Step 2. Materialize Variadic Entry State and `va_list` Layout

Goal: add the narrow RV64 ABI support needed for prepared consumers to receive
or diagnose variadic entry state and `va_list` layout explicitly.

Concrete actions:

- Implement the minimal RV64 target hook changes for variadic entry state.
- Implement the minimal RV64 `va_list` layout publication or a precise
  structured unsupported diagnostic.
- Add focused backend tests for the publication and unsupported paths.

Completion check:

- Focused backend tests prove the RV64 target facts are no longer missing for
  the simple representative path, and non-vararg admission behavior remains
  green.

### Step 3. Materialize `va_start` Helper Resources and Operand Homes

Goal: consume prepared `va_start` facts and provide the helper resources and
destination homes RV64 needs without source-shape inference.

Concrete actions:

- Add target hook support for required scratch register and scratch stack
  resource facts.
- Materialize `va_start.destination_va_list` and
  `va_start.destination_va_list_address` from prepared state.
- Add focused tests that distinguish materialized homes from precise
  unsupported diagnostics.

Completion check:

- `va_start` helper-resource and destination-home diagnostics either disappear
  for covered paths or become more specific target ABI diagnostics.

### Step 4. Materialize or Diagnose Aggregate `va_arg` Operand Homes

Goal: handle aggregate `va_arg` operand homes at the RV64 target boundary using
prepared facts.

Concrete actions:

- Inspect the prepared aggregate `va_arg` access plan and payload homes.
- Materialize `source_va_list`, `aggregate_destination_payload`, and
  `aggregate_access_plan` where RV64 support is narrow and clear.
- Report precise target ABI diagnostics for unsupported aggregate classes.
- Add focused tests for both materialized and unsupported aggregate paths.

Completion check:

- `src/920908-1.c` no longer reports the original aggregate operand-home
  missing facts, or the residual failure identifies a narrower unsupported
  RV64 aggregate vararg class.

### Step 5. Milestone Validation and Handoff

Goal: prove the RV64 vararg target hook work is stable enough to return to the
broader RV64 object-route backlog.

Concrete actions:

- Run build proof and the focused backend tests touched by this plan.
- Run the representative RV64 allowlist for:
  - `src/20030914-2.c`
  - `src/920908-1.c`
- Document any residual target ABI gaps in `todo.md` and create a separate
  idea only if the remaining work is outside this target vararg hook scope.

Completion check:

- The focused backend tests are green, representative outcomes are improved or
  more precisely classified, and no progress depends on testcase-name
  matching or expectation downgrades.
