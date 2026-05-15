# AArch64 I128 Pair Lowering Runbook

Status: Active
Source Idea: ideas/open/236_aarch64_i128_pair_lowering.md
Activated from: ideas/open/236_aarch64_i128_pair_lowering.md

## Purpose

Lower supported AArch64 i128 transport and operations through explicit
low/high pair or memory-backed carriers, selected machine-node records, and
printer output.

## Goal

Establish enough prepared/shared i128 authority for AArch64 lowering to
consume structured low/high or memory-backed facts, then add selected
machine-node support for representative i128 transport, arithmetic,
comparisons, shifts, and helper-call boundaries without recreating archived
accumulator-pair shortcuts.

## Core Rule

AArch64 i128 lowering must consume prepared pair homes, memory carriers,
lane-ordering, helper-call, and clobber/resource facts. Do not create a local
i128 allocator or infer pair homes from rendered register names, fixed
`x0`/`x1` conventions, or named proof cases.

## Read First

- `ideas/open/236_aarch64_i128_pair_lowering.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- prepared value-home, ABI, regalloc, and call-boundary facts for wide integer
  values
- focused backend tests under `tests/backend/mir/`

## Current Targets

- Prepared low/high pair homes or explicit memory-backed i128 carriers.
- Structured low/high lane ordering and lane register facts.
- I128 pair load/store transport records.
- I128 add, sub, bitwise, shift, and comparison records where pair facts are
  complete.
- Runtime helper-call records for div/rem or float-conversion boundaries where
  structured argument, result, and clobber facts exist.
- Printer support that emits only from structured pair operands and prepared
  helper-call facts.

## Non-Goals

- Do not create a separate i128 allocator inside AArch64 codegen.
- Do not infer low/high homes from register names or fixed accumulator pairs.
- Do not lower i128 as scalar i64 or through named testcase shortcuts.
- Do not merge binary128 soft-float, F32/F64 scalar FP, atomics, intrinsics,
  inline asm, callee-save placement, or preserved-value extent work into this
  route.
- Do not weaken unsupported expectations to claim i128 progress.

## Working Model

I128 lowering spans prepared allocation facts and AArch64 selected nodes. This
runbook starts by verifying whether prepared low/high or memory-backed carrier
authority exists. If it is missing, execution must stop with an explicit
prepared/shared blocker. If it is present, add selected records in small
families and print only when every lane, ordering, register, memory, or helper
fact is structured.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Start with inspection; do not jump directly into pair instruction selection.
- Add or validate prepared/shared pair authority before target codegen
  consumes it.
- Preserve fail-closed diagnostics for missing pair homes, lane ordering,
  memory extents, helper-call facts, or clobber/resource authority.
- Treat expectation-only changes, fixed-register matching, scalar-i64
  lowering, and named-case helper shortcuts as route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused AArch64 i128 backend subset. Escalate to broader
  backend validation after shared prepared, call, or printer behavior changes.

## Ordered Steps

### Step 1: Inspect I128 Prepared And AArch64 Surfaces

Goal: determine which i128 carrier, dispatch, helper-call, and printer facts
already exist and which must become prepared/shared blockers before selection.

Primary targets:

- prepared value homes and regalloc classification for `I128`
- ABI and call/return legalization facts for i128
- current AArch64 instruction records, dispatch diagnostics, and printer paths
- runtime helper-call records and clobber/resource facts

Actions:

- Trace representative i128 transport, add/sub, comparison, shift, div/rem,
  and conversion cases from BIR through prepared state into AArch64 dispatch.
- Identify required structured operands: low/high homes, memory carrier,
  lane ordering, source/result pair facts, carry/borrow behavior, shift count,
  comparison signedness, helper arguments/results, and helper clobbers.
- Compare required operands against existing prepared/shared facts.
- Record the first implementation packet target and focused proof subset in
  `todo.md`.

Completion check:

- `todo.md` records whether execution can proceed to i128 pair carrier
  preparation or selected transport, or must split out a narrower
  prepared/shared authority blocker with exact missing facts.

### Step 2: Establish I128 Pair Or Memory Carrier Authority

Goal: ensure values requiring i128 transport have explicit prepared low/high
pair homes or memory-backed carriers before AArch64 lowering consumes them.

Actions:

- Define or complete carrier facts for low/high pair homes and memory-backed
  i128 values.
- Preserve lane ordering, value identity, storage extent, alignment, and
  register/memory ownership as structured facts.
- Add missing-fact diagnostics for incomplete pair or memory carrier state.
- Add focused prepared/record coverage that proves carriers are structural and
  not inferred from names.

Completion check:

- I128 values expose explicit low/high or memory-backed authority that selected
  AArch64 records can consume, or `todo.md` records the exact blocker.

### Step 3: Select I128 Transport Nodes

Goal: lower i128 load/store and pair transport paths from complete prepared
carrier facts.

Actions:

- Add selected records for pair-to-pair, memory-to-pair, pair-to-memory, or
  supported memory-backed transport paths.
- Consume low/high lane homes, memory carrier facts, size, alignment, and lane
  ordering directly.
- Preserve fail-closed diagnostics for incomplete transport authority.
- Add focused dispatch/record coverage for representative transport paths.

Completion check:

- Supported i128 transport emits structured selected records from prepared
  pair or memory facts; unsupported transport remains explicitly diagnosed.

### Step 4: Select I128 Arithmetic And Bitwise Nodes

Goal: lower add, sub, and bitwise i128 pair operations while preserving
low/high lane semantics.

Actions:

- Add selected records for supported add, sub, and bitwise operations.
- Preserve carry/borrow propagation and low/high lane ordering as structured
  fields.
- Consume prepared source/result pair facts directly.
- Keep missing pair homes or unsupported operation shapes fail-closed.
- Add focused coverage for add/sub and at least one bitwise operation.

Completion check:

- Supported i128 arithmetic and bitwise cases produce selected pair records
  without scalar-i64 shortcuts or fixed-register assumptions.

### Step 5: Select I128 Shift And Comparison Nodes

Goal: lower supported i128 shifts and comparisons from structured pair facts.

Actions:

- Add selected records for supported shift operations with explicit shift-count
  operands and lane behavior.
- Add selected comparison records that preserve signed/unsigned high-word
  behavior.
- Consume prepared pair and scalar result facts directly.
- Preserve diagnostics for unsupported shift/count/comparison states.
- Add focused coverage for representative shift and signed/unsigned comparison
  paths.

Completion check:

- Supported i128 shifts and comparisons emit structured selected records with
  correct lane and signedness semantics, or record the exact blocker.

### Step 6: Add Runtime Helper Boundary Records

Goal: represent required i128 runtime helper calls with structured argument,
result, and clobber/resource facts.

Actions:

- Identify div/rem and float-conversion i128 helper paths that require runtime
  calls.
- Add or consume helper-call records for low/high arguments, results, clobbers,
  and resource requirements.
- Preserve helper-call boundaries instead of local marshaling through fixed
  registers.
- Add focused coverage for at least one required helper-call family, or record
  missing helper authority exactly.

Completion check:

- Supported helper-call i128 paths expose structured call boundary records, and
  unsupported helper facts remain fail-closed.

### Step 7: Print Supported I128 Pair Nodes

Goal: print supported i128 pair operations only from complete selected records.

Actions:

- Add printer support for selected transport and simple pair operation records
  that have complete lane/register/memory facts.
- Print helper-call boundaries only from structured helper records.
- Preserve explicit diagnostics for incomplete pair, memory, lane, or helper
  facts.
- Add focused printer coverage for the supported subset.

Completion check:

- Supported i128 selected records print from structured fields; unsupported
  cases do not recover operands from rendered names or fixed conventions.

### Step 8: Validate And Summarize

Goal: prove accepted i128 pair lowering coverage and preserve remaining
blockers at the correct lifecycle layer.

Actions:

- Run the supervisor-chosen build and focused i128 backend subset.
- Escalate to broader backend validation if shared prepared, call, or printer
  behavior changed across multiple operation families.
- Summarize supported transport, arithmetic, comparison, shift, and helper
  paths in `todo.md`.
- Record remaining binary128, scalar FP, atomic, intrinsic, inline-asm, or
  prepared-frame blockers as separate lifecycle candidates rather than
  expanding this route.

Completion check:

- Supported i128 pair paths pass through structured lowering/printing or fail
  with explicit unsupported diagnostics, and no supported case depends on
  name-shaped matching or scalar-i64 shortcuts.
