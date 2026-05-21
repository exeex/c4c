# AArch64 Indexed Aggregate Snapshot Writeback Regression Runbook

Status: Active
Source Idea: ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the current AArch64 dynamic indexed aggregate snapshot/writeback
regression selected by the backend inventory.

## Goal

Make dynamic indexed local and global aggregate element operations preserve the
selected element address and write back through that address instead of stale
snapshots or uninitialized stack homes.

## Core Rule

Repair the general selected aggregate address/writeback boundary. Do not
special-case `00157`, `00176`, one stack offset, one array name, one global
symbol, or one emitted instruction sequence.

## Read First

- `ideas/open/371_aarch64_indexed_aggregate_snapshot_writeback_regression.md`
- `ideas/closed/348_aarch64_indexed_aggregate_address_writeback.md`
- `todo.md`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00157.c`
- `tests/c/external/c-testsuite/src/00176.c`
- generated:
  - `build/c_testsuite_aarch64_backend/src/00157.c.s`
  - `build/c_testsuite_aarch64_backend/src/00176.c.s`

## Current Scope

- `c_testsuite_aarch64_backend_src_00157_c`
- `c_testsuite_aarch64_backend_src_00176_c`
- dynamic local array selected element store/load
- global array swap selected element address/writeback
- focused backend coverage for snapshot/writeback behavior

## Non-Goals

- Do not reopen or edit closed idea 348.
- Do not broaden into scalar FP materialization, conditional/switch arm
  materialization, external/libc return publication, pointer/subobject address
  publication, global `sizeof`, complex initializers, enum bit-fields, ABI, or
  timeout buckets.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not implement under umbrella idea 295.

## Working Model

The backend inventory selected this route because `00157` and `00176` are the
strongest current multi-test family. Both failures show dynamic indexed
aggregate operations lowered through fixed snapshots or stack temporaries
instead of selected element addresses. This is adjacent to closed idea 348, but
fresh generated-code evidence justifies a follow-up owner rather than a
count-only reopen.

## Execution Rules

- Start from generated AArch64 and source-level selected element semantics for
  `00157` and `00176`.
- Trace selected address authority from BIR/prepared records through AArch64
  load/store/writeback emission before editing code.
- Add focused coverage before or with the repair.
- Preserve prior selected-address/writeback repairs from closed idea 348.
- If localization proves one representative has a different first bad fact,
  record that in `todo.md` and keep the implementation route narrow.

## Steps

### Step 1: Localize Snapshot Writeback Boundary

Goal: identify the first backend boundary where dynamic selected aggregate
element address authority is lost.

Primary target: generated `00157.c.s`, generated `00176.c.s`, and prepared
records for selected local/global aggregate element access.

Actions:

- Inspect `00157` source and assembly for the `Array[Count-1] = Count * Count`
  store/load path.
- Inspect `00176` source and assembly for global `array[...]` swap/writeback
  paths.
- Trace whether selected address loss occurs in BIR, prepared aggregate
  access, stack layout, MIR address materialization, or AArch64 load/store
  lowering.
- Compare against closed idea 348's closure boundary so the route does not
  simply repeat prior fixes.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and focused
  coverage requirement for the current snapshot/writeback regression.

### Step 2: Add Focused Snapshot Writeback Coverage

Goal: guard the current selected aggregate snapshot/writeback failure outside
the external representatives.

Primary target: backend tests covering dynamic indexed local and global
aggregate element writeback.

Actions:

- Add or extend focused coverage for a local dynamic indexed element store
  followed by a read from the same selected element.
- Add or extend focused coverage for a global indexed array swap or writeback
  if the localized boundary handles both representatives.
- Assert semantic selected-address behavior, not one emitted offset or
  register spelling.
- Keep coverage independent of `00157`, `00176`, one array name, or one
  stack-offset pattern.

Completion check:

- Focused coverage fails before the repair or directly guards the localized
  selected-address/writeback fact.

### Step 3: Repair Selected Aggregate Address Writeback

Goal: make the localized backend boundary preserve selected aggregate element
addresses and write back through them.

Primary target: the BIR/prepared/MIR/AArch64 helper localized in Step 1.

Actions:

- Implement the smallest semantic repair at the owner boundary.
- Preserve selected element address authority across temporaries, helper
  materializations, calls, and join points required by the representatives.
- Avoid broad rewrites of unrelated scalar, FP, conditional, call-return,
  metadata, initializer, bit-field, ABI, or timeout owners.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 no longer uses stale
  snapshots or uninitialized stack homes for the repaired selected element
  writeback shape.

### Step 4: Prove Representatives And Classify Residuals

Goal: prove the selected owner advanced without hiding a new first bad fact.

Primary target: focused coverage plus
`c_testsuite_aarch64_backend_src_00157_c` and
`c_testsuite_aarch64_backend_src_00176_c`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00157.c.s` and `00176.c.s` enough to confirm selected
  element address/writeback behavior.
- If a representative still fails, classify the new first bad fact in
  `todo.md`.
- Ask the supervisor whether backend-regex or broader regression guard proof
  is needed before closure.

Completion check:

- `00157` and `00176` pass or are reclassified by new first bad facts, and
  regression proof is recorded in `todo.md`.
