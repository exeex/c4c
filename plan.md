# AArch64 I128 Pair Lowering Runbook

Status: Active
Source Idea: ideas/open/236_aarch64_i128_pair_lowering.md
Activated from: ideas/open/236_aarch64_i128_pair_lowering.md
Resumed after prerequisites:
- ideas/closed/248_prepared_i128_runtime_helper_authority.md
- ideas/closed/249_prepared_i128_helper_marshaling_abi_binding.md

## Purpose

Finish the supported AArch64 i128 pair lowering route by consuming structured
helper-boundary marshaling and ABI register-binding facts for terminal div/rem
helper-call printing.

## Goal

Print supported direct-result i128 `SDiv`, `UDiv`, `SRem`, and `URem`
helper-boundary calls from selected records and prepared marshaling facts,
without hard-coded helper ABI registers, rendered-name recovery, or scalar-i64
shortcuts.

## Core Rule

AArch64 i128 helper-call printing must consume structured selected-record,
carrier, ABI binding, marshal/unmarshal, clobber/resource, live-preservation,
and selected-call ownership facts. Do not infer operands from fixed `x0`/`x1`
conventions, register adjacency, opcodes, rendered names, or helper callee
strings.

## Read First

- `ideas/open/236_aarch64_i128_pair_lowering.md`
- `ideas/closed/248_prepared_i128_runtime_helper_authority.md`
- `ideas/closed/249_prepared_i128_helper_marshaling_abi_binding.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- prepared i128 carrier, runtime-helper, marshaling, ABI binding,
  call-clobber, live-preservation, and selected-call ownership facts
- focused backend tests under `tests/backend/mir/`

## Current Targets

- Terminal printing for supported direct-result i128 div/rem helper-boundary
  records.
- Structured marshal moves from prepared carrier lanes to helper ABI argument
  registers.
- Structured terminal `bl <callee>` output from selected-call ownership.
- Structured unmarshal moves from helper ABI result registers to prepared
  result carrier lanes.
- Fail-closed diagnostics for incomplete selected-call ownership,
  live-preservation gaps, wrong carrier shapes, float/i128 conversions, and
  memory-return helper families.

## Non-Goals

- Do not add float/i128 conversion helper mapping.
- Do not add memory-return helper-family support without a separate source
  idea.
- Do not implement generic call lowering, retained-call rewrites, binary128,
  atomics, intrinsics, inline asm, callee-save placement, or preserved-value
  extent work.
- Do not hard-code helper ABI registers in AArch64 printer or dispatch code.
- Do not weaken unsupported expectations to claim helper-call printing.

## Working Model

The earlier i128 runbook established carriers, selected pair operations,
selected div/rem helper-boundary records, and printer support for the
non-helper selected subset. Idea 249 now supplies the missing ABI binding,
marshal/unmarshal, clobber/resource, live-preservation, and selected-call
ownership facts. The next code packet should consume those facts in the
AArch64 printer and keep unsupported helper shapes fail-closed.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Resume at Step 7; do not redo completed carrier, operation-selection, or
  prepared-helper authority work unless a direct dependency is stale.
- Print helper calls only when selected-call ownership and marshaling facts are
  complete.
- Preserve fail-closed diagnostics for incomplete live preservation, missing
  ABI bindings, missing marshal/unmarshal facts, non-direct result ownership,
  float/i128 conversions, and memory-return helpers.
- Treat hard-coded ABI registers, fixed-register matching, scalar-i64
  lowering, target-local helper selection, and expectation-only changes as
  route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused AArch64 i128 backend subset. Escalate to broader
  backend validation after printer-visible helper-call behavior changes.

## Ordered Steps

### Step 1: Inspect I128 Prepared And AArch64 Surfaces

Status: Completed before prerequisite splits.

Completion check:

- Prepared/shared i128 carrier and helper-boundary requirements were recorded.

### Step 2: Establish I128 Pair Or Memory Carrier Authority

Status: Completed before prerequisite splits.

Completion check:

- I128 values expose explicit low/high or memory-backed authority for selected
  AArch64 records.

### Step 3: Select I128 Transport Nodes

Status: Completed before prerequisite splits.

Completion check:

- Supported i128 transport emits structured selected records from prepared
  pair or memory facts.

### Step 4: Select I128 Arithmetic And Bitwise Nodes

Status: Completed before prerequisite splits.

Completion check:

- Supported i128 arithmetic and bitwise cases produce selected pair records
  without scalar-i64 shortcuts or fixed-register assumptions.

### Step 5: Select I128 Shift And Comparison Nodes

Status: Completed before prerequisite splits.

Completion check:

- Supported i128 shifts and comparisons emit structured selected records with
  correct lane and signedness semantics.

### Step 6: Consume Prepared I128 Runtime Helper Boundaries

Status: Completed before idea-249 prerequisite split.

Completion check:

- Supported div/rem helper paths expose structured selected AArch64
  helper-boundary records from prepared facts.

### Step 7: Print I128 Runtime Helper Boundary Calls

Goal: print supported direct-result i128 div/rem helper-boundary calls from
structured selected records and prepared marshaling facts.

Primary targets:

- `I128RuntimeHelperBoundaryRecord`
- AArch64 terminal machine printer helper-boundary path
- selected-call ownership diagnostics
- focused machine-printer and instruction-dispatch tests

Actions:

- Emit marshal moves from prepared carrier lanes to helper ABI argument
  registers using structured marshal facts.
- Emit terminal `bl <callee>` only when selected-call ownership is complete.
- Emit unmarshal moves from helper ABI result registers back to prepared result
  carrier lanes using structured unmarshal facts.
- Consume helper clobber/resource and live-preservation facts as diagnostics or
  selected-record fields required for terminal output.
- Keep incomplete live preservation, missing ABI bindings, wrong carrier
  shapes, float/i128 conversions, and memory-return helper families
  fail-closed.
- Add focused printer coverage for at least one signed and one unsigned
  direct-result div/rem helper path plus incomplete-authority diagnostics.

Completion check:

- Supported direct-result div/rem helper-boundary records print from structured
  fields, and unsupported helper shapes remain explicitly diagnosed.

### Step 8: Validate And Summarize

Goal: prove the accepted i128 pair lowering and helper-call printing coverage
without expanding into deferred helper families.

Actions:

- Run the supervisor-chosen build and focused i128 backend subset.
- Escalate to broader backend validation because terminal helper-call printer
  behavior changed.
- Summarize supported transport, arithmetic, comparison, shift, and direct
  div/rem helper-call printing paths in `todo.md`.
- Record remaining binary128, scalar FP, atomic, intrinsic, inline-asm,
  float/i128 helper, memory-return helper, or prepared-frame blockers as
  separate lifecycle candidates rather than expanding this route.

Completion check:

- Supported i128 pair paths and direct-result div/rem helper paths pass through
  structured lowering/printing or fail with explicit unsupported diagnostics,
  and no supported case depends on name-shaped matching, target-local helper
  selection, hard-coded ABI registers, or scalar-i64 shortcuts.
