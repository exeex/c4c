# RV64 Object Route Prepared Local Memory Addressing Runbook

Status: Active
Source Idea: ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md

## Purpose

Activate the direct follow-up from idea 370 for the new `src/20030914-2.c`
boundary:

```text
unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing
```

## Goal

Lower or precisely route the first supportable RV64 object-route prepared
local-memory addressing shape using explicit prepared metadata.

## Core Rule

Consume prepared frame-slot, pointer-value, offset, size, and alignment facts;
do not reconstruct local-memory bases or aggregate byte ranges from source
syntax, testcase names, physical registers, raw object offsets, or log text.

## Read First

- `ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md`
- The prepared local-memory access structures and diagnostics in the RV64
  object route.
- Existing RV64 object-emission and prepared-printer tests that cover local
  memory, frame slots, pointer-value bases, byval homes, or rejected local
  memory shapes.
- Recent idea 370 closure context for `src/20030914-2.c`, especially the
  transition from `unsupported_byval_param_home` to the local-memory diagnostic.

## Current Targets

- Representative gcc torture case: `src/20030914-2.c`
- Focused backend fixtures for prepared local-memory addressing and rejected
  adjacent shapes
- RV64 object-emission path that currently emits
  `unsupported_local_memory_access`

## Non-Goals

- Do not reopen byval aggregate parameter-home admission; idea 370 closed that
  boundary.
- Do not implement non-register entry parameter homes; that belongs to
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Do not implement aggregate `va_arg` helper lowering; that belongs to
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Do not route unrelated global or data-section materialization here.
- Do not add source-name handling for `src/20030914-2.c`.
- Do not weaken expectations, allowlists, or unsupported contracts to claim
  progress.

## Working Model

The representative now reaches a later object-route boundary after explicit
byval parameter-home facts are admitted. The next repair should inspect the
prepared local-memory access metadata already available to object emission,
choose a fact-complete default-address-space shape, and lower only that shape.
Unsupported widths, missing base facts, non-default address spaces, dynamic
offsets, ambiguous aggregate homes, and out-of-bounds accesses must remain
fail-closed with precise diagnostics.

## Execution Rules

- Keep each implementation packet narrow enough to prove with a build and a
  focused RV64 object-route test subset.
- Prefer new focused backend tests before broad gcc torture scans.
- Preserve or improve the existing unsupported diagnostic when a shape remains
  out of scope.
- Treat testcase-specific matching for `src/20030914-2.c` as route drift.
- Escalate to supervisor review if the needed metadata is not present in the
  prepared contract; do not infer it in target emission.
- Use the canonical `test_after.log` only when delegated by the supervisor for
  executor proof.

## Steps

### Step 1: Audit Local-Memory Metadata

Goal: identify the exact prepared facts available at the current
`unsupported_local_memory_access` boundary.

Primary target: RV64 object-route local-memory rejection path and
`src/20030914-2.c` prepared dumps.

Actions:

- Locate the diagnostic site that emits the current local-memory access
  unsupported message.
- Trace the prepared access facts available there: base kind, frame slot or
  pointer-value identity, offset representation, access size, alignment,
  address space, and aggregate/home provenance.
- Compare those facts with nearby focused backend fixtures to find the
  smallest semantic support shape.
- Record in `todo.md` which shape is supportable and which adjacent shapes
  should remain rejected.

Completion check:

- The executor can name the first supported local-memory addressing shape using
  only prepared facts, and can name the rejected adjacent shapes without
  relying on source syntax or testcase identity.

### Step 2: Add Focused Supported And Rejected Fixtures

Goal: encode the selected support boundary in focused backend tests before or
alongside implementation.

Primary target: RV64 object-emission tests and prepared-printer or diagnostic
coverage for local-memory accesses.

Actions:

- Add or extend focused tests for the selected fact-complete default-address
  space local-memory addressing shape.
- Add adjacent rejected-shape coverage for missing base facts, unsupported
  widths, non-default address spaces, dynamic offsets, ambiguous aggregate
  homes, or out-of-bounds accesses as applicable to the audited boundary.
- Keep test names and expectations semantic; do not key behavior to
  `src/20030914-2.c`.

Completion check:

- The focused tests fail for the missing supported behavior or prove the
  existing fail-closed diagnostics for rejected shapes before the lowering
  change is accepted.

### Step 3: Implement Prepared Local-Memory Addressing Lowering

Goal: lower the selected local-memory access shape in the RV64 object route.

Primary target: RV64 object-route memory-address materialization and emission
helpers.

Actions:

- Add or reuse a helper that materializes the supported frame-slot or
  pointer-value base-plus-offset address from prepared metadata.
- Emit the load/store or address materialization for the selected size and
  alignment class.
- Preserve fail-closed diagnostics for unsupported widths, missing base facts,
  non-default address spaces, dynamic offsets, ambiguous aggregate homes, and
  out-of-bounds accesses.
- Keep any helper boundaries named around semantic prepared facts, not the
  representative testcase.

Completion check:

- Focused supported tests pass because of semantic local-memory addressing
  lowering, while rejected-shape tests still produce precise unsupported
  diagnostics.

### Step 4: Rerun Representative And Route Next Boundary

Goal: prove whether `src/20030914-2.c` advances and document the next owner.

Primary target: the narrow RV64 gcc torture object runner for
`src/20030914-2.c`.

Actions:

- Rerun the representative using the repo-native narrow RV64 gcc torture
  object-route command selected by the supervisor.
- Document whether the case passes or advances to aggregate `va_arg`,
  global/data work, or another distinct owner.
- If a new distinct initiative is exposed, report it to the supervisor instead
  of silently expanding this plan.

Completion check:

- `todo.md` records the representative outcome, the proof command, and whether
  the next boundary belongs to this idea or a separate open idea.

### Step 5: Broader Guard And Closure Review

Goal: prepare the slice for supervisor acceptance and eventual source-idea
closure.

Primary target: existing RV64 object-emission, prepared frame-stack call
contract, and prepared-printer coverage.

Actions:

- Run the focused proof required for the implementation packet.
- Run broader RV64 object-emission or prepared-contract coverage if the helper
  touches shared address materialization.
- Confirm no expectations, allowlists, or unsupported contracts were weakened.
- Summarize remaining unsupported local-memory shapes and why they are outside
  this runbook or need a separate idea.

Completion check:

- The implementation has fresh build/test proof, no testcase-overfit evidence,
  and clear notes for any remaining local-memory addressing gaps.
