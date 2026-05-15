# AArch64 Variadic Machine Node Consumption Runbook

Status: Active
Source Idea: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Activated from: ideas/open/243_aarch64_variadic_machine_node_consumption.md

## Purpose

Consume the prepared AArch64 variadic-entry facts created by idea 232 in
selected machine-node lowering for `va_start`, scalar `va_arg`, aggregate
`va_arg`, and `va_copy`.

## Goal

Produce structured AArch64 machine-node records and printer output for
representative variadic helper paths from complete prepared facts, or stop with
an exact lifecycle blocker naming the missing prepared storage or scratch facts.

## Core Rule

AArch64 lowering may consume prepared variadic facts, but must not reconstruct
AAPCS64 `va_list` layout, register-save areas, overflow-area offsets, named
argument counts, or scratch-resource policy locally.

## Read First

- `ideas/open/243_aarch64_variadic_machine_node_consumption.md`
- `ideas/closed/232_aarch64_variadic_function_entry_carriers.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/variadic.md`
- focused tests under `tests/backend/mir/` and variadic C cases under
  `tests/c/internal/abi/`

## Current Targets

- `PreparedVariadicEntryPlanFunction` and related helper-resource facts.
- AArch64 helper-call dispatch that currently recognizes variadic helpers and
  then defers lowering.
- Selected machine-node records for `va_start`, scalar `va_arg`, aggregate
  `va_arg`, and `va_copy`.
- Printer support that emits only from structured operands and prepared storage
  facts.
- Focused tests that distinguish prepared-carrier presence from real machine
  node consumption.

## Non-Goals

- Do not infer register-save slots, stack offsets, overflow bases, `va_list`
  layout, named GP/FP counts, or scratch allocation inside AArch64 codegen.
- Do not weaken fail-closed diagnostics or mark unsupported helper cases as
  supported without selected machine-node evidence.
- Do not claim support through prepared dump coverage alone.
- Do not broaden into global address, memory load/store, scalar cast, i128,
  binary128, atomic, intrinsic, inline-asm, callee-save slot-placement, or
  preserved-value extent work.
- Do not rewrite the prepared-carrier source idea unless a genuine lifecycle
  blocker proves the source intent must change.

## Working Model

Idea 232 made variadic callee-entry metadata visible and guarded. This runbook
starts by checking whether those facts are complete enough for target lowering.
If they are complete, add selected nodes and printer paths one helper family at
a time. If they are incomplete, keep AArch64 fail-closed behavior and record the
smallest missing prepared/shared fact as the next lifecycle initiative.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Prefer small packets: inspect, consume `va_start`, consume scalar `va_arg`,
  consume aggregate `va_arg`, consume `va_copy`, then validate.
- Add or strengthen focused tests before relying on broader backend validation.
- Treat expectation-only changes, fixture-name matching, unsupported
  downgrades, or diagnostic-only rewrites as route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused AArch64 variadic subset. Escalate to broader
  backend validation after shared prepared or printer behavior changes.
- If complete storage or scratch authority is missing, stop at an explicit
  blocker rather than filling the gap in AArch64 target codegen.

## Ordered Steps

### Step 1: Inspect Prepared Variadic Consumption Boundary

Goal: Determine which prepared variadic facts are already complete enough for
machine-node consumption and which helper paths still need new prepared/shared
authority.

Primary targets:

- `PreparedVariadicEntryPlanFunction`
- AArch64 helper-call dispatch and call-record construction
- current `deferred_unsupported` variadic diagnostics
- focused variadic record and printer tests

Actions:

- Trace current `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`
  helper records from BIR through prepared state into AArch64 dispatch.
- Identify the exact structured operands each helper needs: destination homes,
  `va_list` storage, register-save-area access, overflow-area progression,
  aggregate payload storage, copy source/destination, and scratch resources.
- Compare required operands against the facts exposed by idea 232.
- Record the initial proof subset and any blockers that must become separate
  prepared/shared initiatives.

Completion check:

- `todo.md` records whether execution can proceed to `va_start` consumption or
  must split out a narrower prepared-fact blocker, with exact missing fields and
  focused tests named.

### Step 2: Consume Prepared Facts For `va_start`

Goal: Lower a representative `va_start` path into selected machine nodes from
prepared entry facts.

Actions:

- Add or extend machine-node records for `va_list` field initialization and
  register-save/overflow base materialization.
- Consume prepared field layout, register-save-area, overflow-area, named
  register counts, and helper-resource facts directly.
- Preserve fail-closed diagnostics for incomplete prepared facts.
- Add focused record and printer coverage for the supported `va_start` path.

Completion check:

- A supported `va_start` fixture produces structured selected machine records
  and printer output; missing prepared facts still diagnose explicitly.

### Step 3: Consume Prepared Facts For Scalar `va_arg`

Goal: Lower selected scalar `va_arg` paths without target-local ABI
reconstruction.

Actions:

- Add typed machine-node records for GP and FP scalar argument fetches.
- Consume prepared register-save-area and overflow-area progression facts.
- Preserve value-width, alignment, and destination-home facts as structured
  operands.
- Add tests that cover at least one register-backed and one overflow-backed
  scalar access, or record the exact missing prepared fact.

Completion check:

- Scalar `va_arg` lowering either emits structured machine records and printer
  output from prepared facts or stops with a narrow blocker.

### Step 4: Consume Prepared Facts For Aggregate `va_arg`

Goal: Lower aggregate `va_arg` helper effects while preserving aggregate
storage and copy semantics.

Actions:

- Add records for aggregate payload source selection and destination copy.
- Consume prepared aggregate size, alignment, result storage, register-save
  access, and overflow progression facts.
- Keep full-width aggregate transport separate from scalar shortcuts.
- Add focused tests for an aggregate variadic argument path.

Completion check:

- Aggregate `va_arg` either produces structured machine-node effects from
  prepared facts or records an exact prepared extent/storage blocker.

### Step 5: Consume Prepared Facts For `va_copy`

Goal: Lower `va_copy` as a structured copy of the prepared `va_list` layout.

Actions:

- Add machine-node records for copying each supported `va_list` field.
- Consume prepared source/destination homes and field layout facts directly.
- Preserve explicit diagnostics for incomplete storage or layout facts.
- Add focused tests for `va_copy` record and printer output.

Completion check:

- `va_copy` emits structured copy effects from prepared facts, or the route
  records the exact missing prepared source/destination storage authority.

### Step 6: Validate And Summarize

Goal: Prove the accepted consumption route and preserve any remaining blockers
at the correct lifecycle layer.

Actions:

- Run the supervisor-chosen build and focused variadic backend subset.
- Escalate to broader `backend_` validation if selected machine records,
  printer behavior, or prepared facts changed beyond one helper family.
- Summarize supported helper paths, remaining unsupported cases, and any split
  prepared/shared initiatives in `todo.md`.
- Ask the supervisor to decide whether the source idea is complete, blocked, or
  should remain active for another runbook rewrite.

Completion check:

- Supported variadic helper paths have structured selected machine-node and
  printer proof, and remaining gaps are explicit non-overfit lifecycle notes.
