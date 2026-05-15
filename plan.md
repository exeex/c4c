# AArch64 I128 Pair Lowering Runbook

Status: Active
Source Idea: ideas/open/236_aarch64_i128_pair_lowering.md
Activated from: ideas/open/236_aarch64_i128_pair_lowering.md
Resumed after prerequisite: ideas/closed/248_prepared_i128_runtime_helper_authority.md

## Purpose

Lower supported AArch64 i128 transport and operations through explicit
low/high pair or memory-backed carriers, selected machine-node records, and
printer output.

## Goal

Resume selected AArch64 i128 helper-boundary consumption using the prepared
div/rem helper authority supplied by idea 248, then continue to printer and
validation only for supported structured i128 pair records.

## Core Rule

AArch64 i128 lowering must consume prepared pair homes, memory carriers,
lane-ordering, helper-call, and clobber/resource facts. Do not create a local
i128 allocator or infer pair homes, helper callees, lane bindings, or clobbers
from rendered register names, fixed `x0`/`x1` conventions, opcodes, or named
proof cases.

## Read First

- `ideas/open/236_aarch64_i128_pair_lowering.md`
- `ideas/closed/248_prepared_i128_runtime_helper_authority.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- prepared value-home, ABI, regalloc, i128 carrier, and runtime-helper facts
  for wide integer values
- focused backend tests under `tests/backend/mir/`

## Current Targets

- Selected AArch64 helper-boundary records for supported i128 div/rem helpers.
- Prepared div/rem helper facts from `PreparedI128RuntimeHelper`:
  source operation identity, helper kind, callee, low/high argument lanes,
  direct low/high result lanes, clobbers/resources, ABI, and register-bank
  policy.
- I128 pair records already selected by earlier runbook steps.
- Printer support that emits only from structured pair operands and prepared
  helper-boundary facts.

## Non-Goals

- Do not create a separate i128 allocator inside AArch64 codegen.
- Do not infer low/high homes, helper callees, lane ownership, clobbers, or
  ABI policy from register names, opcodes, or fixed accumulator pairs.
- Do not lower i128 as scalar i64 or through named testcase shortcuts.
- Do not implement float/i128 conversion helper mapping or memory-return helper
  families unless a separate prepared/shared authority route supplies those
  facts.
- Do not merge binary128 soft-float, F32/F64 scalar FP, atomics, intrinsics,
  inline asm, callee-save placement, or preserved-value extent work into this
  route.
- Do not weaken unsupported expectations to claim i128 progress.

## Working Model

Earlier steps established direct pair/memory carrier consumption for selected
i128 transport and simple pair operations. Idea 248 now provides prepared
runtime-helper authority for supported div/rem helpers. Step 6 should consume
those facts into selected AArch64 helper-boundary records without synthesizing
helper calls locally. Step 7 printer work remains limited to selected records
whose pair, lane, memory, helper, and clobber/resource facts are complete.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Resume at Step 6; do not redo completed carrier, transport, arithmetic,
  shift, or comparison work unless a local check proves a direct dependency.
- Consume prepared div/rem helper records directly.
- Preserve fail-closed diagnostics for missing pair homes, lane ordering,
  memory extents, helper-call facts, clobber/resource authority, float/i128
  conversions, and memory-return helper families.
- Treat expectation-only changes, fixed-register matching, scalar-i64
  lowering, target-local helper selection, and named-case helper shortcuts as
  route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused AArch64 i128 backend subset. Escalate to broader
  backend validation after shared prepared, call, or printer behavior changes.

## Ordered Steps

### Step 1: Inspect I128 Prepared And AArch64 Surfaces

Status: Completed before the idea-248 prerequisite split.

Completion check:

- `todo.md` recorded the prepared/shared i128 carrier and helper-boundary
  facts needed before selected AArch64 consumption.

### Step 2: Establish I128 Pair Or Memory Carrier Authority

Status: Completed before the idea-248 prerequisite split.

Completion check:

- I128 values expose explicit low/high or memory-backed authority for selected
  AArch64 records.

### Step 3: Select I128 Transport Nodes

Status: Completed before the idea-248 prerequisite split.

Completion check:

- Supported i128 transport emits structured selected records from prepared
  pair or memory facts; unsupported transport remains explicitly diagnosed.

### Step 4: Select I128 Arithmetic And Bitwise Nodes

Status: Completed before the idea-248 prerequisite split.

Completion check:

- Supported i128 arithmetic and bitwise cases produce selected pair records
  without scalar-i64 shortcuts or fixed-register assumptions.

### Step 5: Select I128 Shift And Comparison Nodes

Status: Completed before the idea-248 prerequisite split.

Completion check:

- Supported i128 shifts and comparisons emit structured selected records with
  correct lane and signedness semantics, or record the exact blocker.

### Step 6: Consume Prepared I128 Runtime Helper Boundaries

Goal: add selected AArch64 helper-boundary records for supported i128 div/rem
helpers from prepared runtime-helper authority.

Primary targets:

- `PreparedI128RuntimeHelper` div/rem records
- selected AArch64 helper-boundary records
- dispatch diagnostics for unsupported helper-required operations
- focused target-record and dispatch tests

Actions:

- Consume source operation identity, helper kind, and callee facts for
  `SDiv`, `UDiv`, `SRem`, and `URem`.
- Consume low/high argument lane bindings and direct low/high result lane
  bindings from prepared i128 carriers.
- Preserve helper-specific clobber/resource policy, ABI facts, and GPR
  argument/result bank facts as structured selected-record fields.
- Keep float/i128 conversions and memory-return helper families fail-closed
  until separate prepared/shared authority exists.
- Add focused selected-record and dispatch coverage for at least one signed and
  one unsigned div/rem helper path.

Completion check:

- Supported div/rem helper paths expose structured selected AArch64
  helper-boundary records from prepared facts, and unsupported helper-required
  operations remain explicitly diagnosed.

### Step 7: Print Supported I128 Pair Nodes

Goal: print supported i128 pair and helper-boundary operations only from
complete selected records.

Actions:

- Add printer support for selected transport and simple pair operation records
  that have complete lane/register/memory facts.
- Print supported div/rem helper boundaries only from structured helper
  records.
- Preserve explicit diagnostics for incomplete pair, memory, lane, helper, or
  clobber/resource facts.
- Add focused printer coverage for the supported subset.

Completion check:

- Supported i128 selected records print from structured fields; unsupported
  cases do not recover operands from rendered names, fixed conventions, or
  opcodes.

### Step 8: Validate And Summarize

Goal: prove accepted i128 pair lowering coverage and preserve remaining
blockers at the correct lifecycle layer.

Actions:

- Run the supervisor-chosen build and focused i128 backend subset.
- Escalate to broader backend validation if shared prepared, call, or printer
  behavior changed across multiple operation families.
- Summarize supported transport, arithmetic, comparison, shift, and div/rem
  helper paths in `todo.md`.
- Record remaining binary128, scalar FP, atomic, intrinsic, inline-asm,
  float/i128 helper, memory-return helper, or prepared-frame blockers as
  separate lifecycle candidates rather than expanding this route.

Completion check:

- Supported i128 pair paths pass through structured lowering/printing or fail
  with explicit unsupported diagnostics, and no supported case depends on
  name-shaped matching, target-local helper selection, or scalar-i64 shortcuts.
