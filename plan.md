# Prepared Local Address Base-Plus-Offset Boundary Evidence Runbook

Status: Active
Source Idea: ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md

## Purpose

Determine whether retained `prepared_local_address_base_plus_offset_missing`
`unsupported_local_memory_access` rows are prepared-owned or RV64-owned before
any lowering repair proceeds.

## Goal

Name the first bad prepared/RV64 fact for one retained representative, then
repair only the proven boundary if the evidence justifies implementation.

## Core Rule

Do not recover frame-slot, pointer-value, or offset facts from RV64
target-specific instruction shapes. RV64 local-memory lowering must consume
prepared base-plus-offset facts, and prepared ownership must be proven from
focused prepared evidence.

## Read First

- `ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md`
- Existing BIR/prepared/RV64 local-memory tests and dumps
- Producer and consumer code around `PreparedMemoryAccess`,
  `PreparedAddressBaseKind`, address materialization,
  `fragment_for_prepared_store_local`, and
  `fragment_for_prepared_load_local`
- Current RV64 gcc torture artifacts for representatives such as
  `src/20000519-1.c`, `src/20070212-3.c`, `src/pr35800.c`, and
  `src/pr65369.c`, if present

## Current Targets

- Retained local-memory rows diagnosed as
  `prepared_local_address_base_plus_offset_missing`.
- The prepared contract fields required by RV64 local-memory consumers:
  base kind, base-plus-offset usability, size, alignment,
  default-space/non-volatile status, pointer-value register homes, and
  immediate encodability.
- The first failing access in a representative same-snapshot semantic BIR,
  prepared BIR, and RV64 object-route reproduction.

## Non-Goals

- Do not combine this route with direct-call metadata repair.
- Do not reclassify stale summary-file rows without a fresh matching scan.
- Do not weaken unsupported accounting, expected output, tests, or prepared
  admission contracts.
- Do not add named-case shortcuts for any retained torture representative.
- Do not broadly rewrite local-memory lowering outside the named first bad
  prepared/RV64 fact.

## Working Model

- If the required prepared base-plus-offset fact is absent or incoherent, the
  first owner is prepared publication or address materialization.
- If the required prepared fact is present but
  `fragment_for_prepared_store_local` or `fragment_for_prepared_load_local`
  still rejects it, the first owner is the RV64 prepared local-memory consumer.
- If focused dumps do not expose the required fact path, the row remains an
  evidence gap rather than becoming an implementation route.

## Execution Rules

- Keep routine reproduction notes, commands, failing accesses, and proof
  results in `todo.md`.
- Add focused BIR/prepared coverage before relying on representative RV64 proof
  for any code-changing repair.
- Prefer the smallest representative reproduction that shows semantic BIR,
  prepared BIR, and RV64 evidence from the same workspace snapshot.
- Treat diagnostic renames, expectation rewrites, unsupported downgrades, and
  helper-only refactors as non-progress.
- Escalate to a plan-owner split if evidence proves a separate initiative
  outside this prepared/RV64 boundary.

## Steps

### Step 1: Reproduce Local Address Evidence

Goal: capture same-snapshot semantic BIR, prepared BIR, and RV64 object-route
evidence for one retained representative.

Primary target: `src/20000519-1.c`, with `src/20070212-3.c`,
`src/pr35800.c`, or `src/pr65369.c` as fallback representatives.

Actions:
- Find or rerun the current representative command through semantic BIR,
  prepared BIR, and RV64 object routing.
- Record the exact command, failing access, diagnostic text, and relevant dump
  paths in `todo.md`.
- Identify the BIR `load_local` or `store_local` address record that
  corresponds to the failing prepared/RV64 access.
- Stop without implementation if the reproduction cannot tie one failing access
  across the three routes.

Completion check:
- `todo.md` names the representative, command, failing access, semantic BIR
  address record, prepared evidence location, and RV64 rejection point.

### Step 2: Classify The Prepared/RV64 Boundary

Goal: decide whether the first bad fact is prepared-owned, RV64-owned, or still
an evidence gap.

Primary target: prepared `memory_access` and `address_materialization` rows
plus RV64 prepared load/store fragments.

Actions:
- Inspect the focused dumps for `PreparedAddressBaseKind`,
  `can_use_base_plus_offset`, size, alignment, default-space/non-volatile
  status, pointer-value register homes, and immediate encodability.
- Classify prepared-owned only if the needed prepared fact is missing or
  incoherent.
- Classify RV64-owned only if the needed prepared fact is present and rejected
  by `fragment_for_prepared_store_local` or
  `fragment_for_prepared_load_local`.
- Record the classification and first bad fact in `todo.md`; if neither owner
  is proven, preserve the evidence gap and request route review.

Completion check:
- `todo.md` contains a single first-owner classification tied to visible
  same-run prepared and RV64 evidence, or explicitly records why the route is
  blocked as an evidence gap.

### Step 3: Add Focused Contract Coverage

Goal: make the proven boundary observable in focused tests before changing the
repair surface.

Primary target: the smallest BIR/prepared or RV64 prepared-consumer test that
exposes the missing fact or rejected fact.

Actions:
- Add or extend focused tests for the classified fact, preserving existing
  unsupported contracts.
- Keep the test independent of the named torture case when possible.
- Prove the focused test fails before the repair or otherwise document why the
  existing test already covers the exact fact.
- Do not start implementation until the proof surface is clear.

Completion check:
- Focused coverage exists for the classified prepared/RV64 boundary and
  `todo.md` records the exact proof command and result.

### Step 4: Repair Only The Named Boundary

Goal: fix the proven prepared publication or RV64 consumer rejection without
target-shaped reconstruction.

Primary target: the code surface classified in Step 2.

Actions:
- For prepared-owned failures, repair publication or address materialization so
  the required base-plus-offset fact is present and coherent.
- For RV64-owned failures, repair the prepared local-memory consumer so it
  accepts the already-published fact.
- Keep diagnostics fail-closed for malformed or unsupported accesses.
- Avoid broad local-memory rewrites and named-case checks.

Completion check:
- The focused coverage from Step 3 passes, and the diff shows a semantic
  prepared/RV64 boundary repair rather than an expectation or diagnostic-only
  change.

### Step 5: Prove Representative And Broader Behavior

Goal: show that the repair advances representative RV64 behavior and does not
regress adjacent local-memory paths.

Primary target: the representative from Step 1 plus nearby local-memory
prepared/RV64 tests selected by the supervisor.

Actions:
- Rerun the representative RV64 command from Step 1.
- Run the focused test subset from Step 3 and the supervisor-selected broader
  local-memory subset.
- Record commands and results in `todo.md`.
- If the representative still fails for a different first bad fact, record the
  new owner instead of expanding the slice silently.

Completion check:
- `todo.md` contains fresh proof for focused coverage and representative RV64
  behavior, plus any remaining first-owner facts that require a separate route.
