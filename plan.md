# AArch64 Function Pointer Table Relocation Dispatch Runbook

Status: Active
Source Idea: ideas/open/380_aarch64_function_pointer_table_relocation_dispatch.md
Switched From: ideas/closed/379_aarch64_local_aggregate_copy_load_publication.md

## Purpose

Repair the remaining `00216.c` mismatch where a global function-pointer table
with multiple relocation entries dispatches every indexed call to the same
function.

## Goal

Make AArch64 lowering preserve distinct function-pointer table relocation
targets so `test_multi_relocs` prints `one`, `two`, `three` in order.

## Core Rule

Fix the semantic/backend lowering rule for multi-entry function-pointer table
relocations. Do not special-case `00216`, `test_multi_relocs`, one table
symbol, one callee name, one relocation index, one label, or one emitted
instruction sequence.

## Read First

- `ideas/open/380_aarch64_function_pointer_table_relocation_dispatch.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00216.c`
- `build/c_testsuite_aarch64_backend/src/00216.c.s`
- relevant global initializer, relocation, and AArch64 indirect-call lowering
  tests

## Current Scope

- `00216.c` `test_multi_relocs`
- `const fptr table[3]` with range initialization followed by designated
  overwrites for `sys_one`, `sys_two`, and `sys_three`
- global function-pointer table object layout and relocation records
- AArch64 address materialization and indexed indirect-call dispatch from a
  relocated function-pointer table

## Non-Goals

- Do not reopen idea 379 local aggregate address/copy/load publication unless
  fresh evidence proves a shared boundary.
- Do not include timeout-only `00200` or `00207` routes.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, CTest registration, proof-log policy, c-testsuite
  source, or external test contracts.
- Do not claim progress through helper renames, emitted-text reshuffling, or
  classification-only changes.

## Working Model

The Step 5 follow-up under idea 379 fixed the aggregate-publication first bad
facts in `00216`: `ls21`, `lu22`, `lv3`, and `flow` now match expected output.
The delegated proof still fails `c_testsuite_aarch64_backend_src_00216_c` with
a runtime mismatch because `test_multi_relocs` prints `two/two/two` instead of
`one/two/three`. All 146 backend tests pass in the same proof command.

Treat `00216` as a representative and advancement proof, not as the only
contract. Add or use focused backend coverage that demonstrates distinct
function-pointer table relocation targets independently of the external
filename.

## Execution Rules

- Localize the first boundary where `table[0]`, `table[1]`, and `table[2]`
  collapse to the wrong relocation target before changing lowering.
- Trace through semantic BIR, prepared BIR, MIR/global data, generated
  relocation records, and AArch64 assembly as needed.
- Prefer existing global initializer, relocation, and AArch64 indirect-call
  helper patterns over a new one-off path.
- Keep proof narrow while repairing, then run the supervisor-delegated broader
  guard once the focused failure advances.
- If the first bad fact changes after repair, classify the new failure in
  `todo.md` and stop for supervisor routing when it leaves this source idea.

## Steps

### Step 1: Localize Table Relocation First Bad Fact

Goal: identify the exact initializer, relocation, object-layout,
address-materialization, or indirect-call boundary that collapses distinct
`table` entries into the observed `two/two/two` dispatch.

Primary target: `tests/c/external/c-testsuite/src/00216.c`,
`build/c_testsuite_aarch64_backend/src/00216.c.s`, and compiler dumps for
`test_multi_relocs`.

Actions:

- Reproduce or inspect the current `test_multi_relocs` runtime mismatch.
- Trace `table[0]`, `table[1]`, and `table[2]` from semantic initializer
  representation through prepared BIR, MIR/global data, emitted relocation
  records, and generated AArch64 address materialization.
- Decide whether the first bad fact is range-initializer overwrite order,
  relocation target publication, global object layout, pointer-sized element
  addressing, or indirect-call lowering.
- Record the first bad fact and the smallest focused backend shape needed to
  cover it.

Completion check:

- `todo.md` names the exact table/relocation dispatch boundary and the focused
  coverage shape to add or update before repair.

### Step 2: Add Focused Coverage

Goal: create backend coverage for distinct function-pointer table relocation
targets without depending on `00216.c` by name.

Primary target: focused backend/AArch64 tests for global initializers,
relocations, or indirect-call dispatch.

Actions:

- Add a minimal focused case for a multi-entry function-pointer table whose
  entries dispatch to different functions.
- Include range-initializer overwrite behavior if Step 1 localizes ownership to
  designated/range initializer ordering.
- Keep the test semantic enough to reject filename, symbol-name, index,
  relocation-label, or exact-instruction matching.
- Verify the focused case fails before the repair for the same table/relocation
  reason.

Completion check:

- Focused coverage exists and fails before the repair for the localized
  function-pointer table relocation dispatch gap.

### Step 3: Repair Table Relocation Dispatch

Goal: make AArch64 lowering preserve the distinct function targets stored in a
multi-entry function-pointer table.

Primary target: the localized semantic/backend lowering boundary from Step 1.

Actions:

- Apply the smallest general repair at the table initializer, relocation,
  global-data, address-materialization, or indirect-call boundary.
- Reuse existing global initializer and relocation helper conventions.
- Preserve idea 379 aggregate-publication outputs and existing backend
  guardrails.
- Avoid broad rewrites of unrelated global initializer or call lowering logic.

Completion check:

- The focused coverage from Step 2 passes without special-casing names,
  indexes, labels, or emitted instruction neighborhoods.

### Step 4: Prove Advancement And Reclassify

Goal: prove `00216` advances past the current `test_multi_relocs`
`two/two/two` mismatch and classify any new first bad fact without expanding
this owner silently.

Primary target: supervisor-delegated focused and broader backend proof.

Actions:

- Run the supervisor-delegated build and focused backend test command.
- Confirm `c_testsuite_aarch64_backend_src_00216_c` no longer reports
  `two/two/two` for `test_multi_relocs`.
- Confirm the aggregate outputs repaired under idea 379 still match expected
  output.
- If `00216` still fails, classify the new first bad fact in `todo.md`.
- Stop for lifecycle routing if the new owner is timeout-only `00200`/`00207`
  or another parked bucket.

Completion check:

- `todo.md` records focused proof, broader proof if delegated, and either
  acceptance for idea 380 or a new first-bad-fact handoff note.
