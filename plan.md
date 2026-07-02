# Out-Of-SSA Parallel-Copy Move-Bundle Publication Runbook

Status: Active
Source Idea: ideas/open/554_out_of_ssa_parallel_copy_move_bundle_publication.md

## Purpose

Repair the prepared value-location producer path that should publish
out-of-SSA parallel-copy move bundles with coordinates matching prepared
traversal events.

## Goal

Make the `src/960209-1.c` row move past the audited
`prepared_consumer_category=missing_move_bundle` blocker by publishing or
propagating the correct out-of-SSA parallel-copy move-bundle facts.

## Core Rule

Do not infer missing bundles in the prepared consumer and do not special-case
the testcase, block label, predecessor, successor, or block index. The fix must
repair generalized phase, authority, execution-block, predecessor, and
successor coordinate publication.

## Read First

- `ideas/open/554_out_of_ssa_parallel_copy_move_bundle_publication.md`
- `ideas/closed/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/prealloc/prepared_object_traversal.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`

## Current Targets

- Current row: `src/960209-1.c`
- Current diagnostic: `prepared_consumer_category=missing_move_bundle`
- Event kind: `pre_terminator_copies`
- Event block index: `15`
- Prepared block label: `20`
- Parallel-copy edge: predecessor `20`, successor `19`
- Execution block label: `20`
- Lookup execution block index: `15`
- Existing candidate shape: three
  `authority_out_of_ssa_parallel_copy` candidates, zero execution-block
  matches, zero predecessor-label matches, one successor-label match, zero
  exact matches

## Non-Goals

- Do not implement RV64 materialization for the eventual move sequence in this
  plan.
- Do not route this row to F128 quarantine unless new row-level facts prove it
  is F128-primary after the move bundle is found.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not expand this into a broad value-location or prepared traversal rewrite
  beyond the audited coordinate-publication mismatch.

## Working Model

- Prepared traversal is consuming a legitimate pre-terminator parallel-copy
  event.
- Value locations contain some out-of-SSA parallel-copy bundle candidates, but
  their coordinates do not match the event that needs a bundle.
- The first repair target is producer/fact propagation for move-bundle
  coordinates, not consumer-side target-shape handling.

## Execution Rules

- Keep packet progress and proof commands in `todo.md`.
- Preserve the structured missing-bundle diagnostic so follow-up blockers stay
  auditable.
- Prefer focused backend contract tests for producer publication behavior
  before relying on the one-row torture scan.
- Any code-changing packet needs fresh build proof and the delegated focused
  proof command from the supervisor.
- Reject progress that changes only diagnostic text while leaving the same
  missing publication facts.

## Steps

### Step 1: Reproduce And Map Producer Facts

Goal: identify the producer path and current coordinate facts for
out-of-SSA parallel-copy move bundles.

Actions:

- Re-run or inspect the one-row `src/960209-1.c` failure to confirm the current
  missing-bundle evidence.
- Trace where `authority_out_of_ssa_parallel_copy` move bundles are created,
  keyed, and attached to value-location state.
- Compare producer coordinates with the prepared traversal event coordinates:
  phase, authority, execution block, predecessor, and successor.
- Record whether the mismatch is caused by missing execution-block
  propagation, predecessor/successor inversion or loss, stale block labels, or
  a different producer ownership bug.

Completion check:

- `todo.md` names the producer path, the first bad coordinate fact, and the
  minimal repair surface.
- No semantic repair is made until the first bad producer fact is known.

### Step 2: Repair Coordinate Publication

Goal: publish out-of-SSA parallel-copy move bundles with coordinates that
match the prepared traversal event consuming them.

Actions:

- Update the producer or propagation point that attaches execution block,
  predecessor, and successor coordinates to out-of-SSA parallel-copy bundles.
- Keep the rule generalized across blocks and edges; do not special-case
  `src/960209-1.c` or the labels observed in the evidence row.
- Add or update focused backend tests for the coordinate contract.
- Preserve existing behavior for block-entry move bundles and unrelated move
  authorities.

Completion check:

- Focused tests prove out-of-SSA parallel-copy bundles carry matching event
  coordinates.
- The old `candidate_execution_block_count=0` and
  `candidate_predecessor_label_count=0` shape is no longer present for the
  repaired scenario unless the row exposes a new deeper producer blocker.

### Step 3: Prove The Row Moves Past MissingMoveBundle

Goal: verify the repaired publication path against `src/960209-1.c` without
weakening pass/fail accounting.

Actions:

- Run the delegated build and one-row RV64 gcc torture backend scan.
- Inspect the case log for the current first blocker.
- If the row advances to RV64 materialization, F128, or another producer
  blocker, record the new auditable owner in `todo.md` and request lifecycle
  routing instead of expanding this plan silently.

Completion check:

- `test_after.log` records build proof and focused row proof.
- `todo.md` states whether `missing_move_bundle` is fixed or replaced by a
  different row-level first blocker.
- No expectations, unsupported markers, allowlists, or runtime comparison
  behavior were weakened.
