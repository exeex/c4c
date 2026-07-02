# Move-Bundle Target-Shape Evidence Gap For src/960209-1.c

Status: Closed
Type: Evidence reconstruction queue
Parent: `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
Owning Layer: Evidence and first-owner reconstruction

## Goal

Gather enough row-level evidence for `src/960209-1.c` to classify its current
`unsupported_move_bundle_target_shape` failure without guessing first
ownership.

## Why This Exists

The bucket split review found exactly one evidence-gap row. Its current log
confirms membership in the move-bundle target-shape bucket but omits the event
kind, phase, authority, coordinate, value ids, homes, types, and F128-screenable
details needed to choose RV64, prepared, BIR, or F128 ownership.

## Evidence Anchor

- Source splitter: `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
- Classification table:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- Classification note:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
- Lane filter: `first_owner_lane=evidence_gap`
- Current row count: 1
- Current case: `src/960209-1.c`

## Closure Summary

Closed because the evidence gap is satisfied. Step 3 produced auditable
row-level evidence showing that prepared traversal has a
`pre_terminator_copies` event, but value locations do not publish a matching
out-of-SSA parallel-copy move bundle for the event coordinates.

Classification:

- Owner: prepared value-location move-bundle producer/fact propagation.
- Current diagnostic: `prepared_consumer_category=missing_move_bundle`.
- Event: `event_kind=pre_terminator_copies`, `event_block_index=15`,
  `prepared_block_label=20`.
- Parallel-copy coordinates: `parallel_copy_predecessor=20`,
  `parallel_copy_successor=19`, `parallel_copy_execution_block=20`,
  `lookup_execution_block_index=15`.
- Candidate facts: `candidate_move_bundle_count=21`,
  `candidate_phase_block_entry_count=3`,
  `candidate_authority_out_of_ssa_parallel_copy_count=3`,
  `candidate_execution_block_count=0`,
  `candidate_predecessor_label_count=0`,
  `candidate_successor_label_count=1`,
  `candidate_exact_parallel_copy_match_count=0`.
- Value/home/type/F128 facts:
  `value_home_type_f128_facts=unavailable_at_missing_move_bundle`.

Durable follow-up:

- `ideas/open/554_out_of_ssa_parallel_copy_move_bundle_publication.md`

## In Scope

- Reproduce or regenerate diagnostics for `src/960209-1.c` with enough detail
  to identify event kind, phase, authority, move coordinates, value ids,
  source and destination homes, scalar types, and F128 screening facts.
- Classify the row into exactly one first-owner lane after evidence is
  available.
- Route the row to the correct existing or new implementation idea only after
  classification is auditable.

## Out Of Scope

- Implementing RV64 lowering before ownership is known.
- Prepared or BIR repair based only on testcase shape.
- F128 routing without row-level F128 evidence.
- Expectation rewrites, unsupported marker edits, allowlist filtering, or
  runtime comparison changes.

## Acceptance Criteria

- The row has a durable evidence artifact that names the missing facts or
  supplies them.
- The row is reclassified into RV64 materialization, prepared authority, BIR
  producer, F128 quarantine, or a still-explicit evidence gap with a narrower
  blocker.
- Any follow-up implementation route cites the new evidence instead of the
  testcase name.

## Reviewer Reject Signals

- Reject assigning `src/960209-1.c` to RV64, prepared, BIR, or F128 work from
  filename, source shape, raw BIR shape, expected register spelling, or bucket
  membership alone.
- Reject claiming progress through diagnostic text changes that still omit
  event kind, phase, authority, move coordinates, value ids, homes, types, or
  F128-screenable facts.
- Reject unsupported downgrades, expectation rewrites, allowlist filtering, or
  weaker runtime comparison as evidence of resolving this row.
- Reject combining this evidence row with the 151-row RV64 materialization
  queue or the 31-row prepared authority queue before classification is
  auditable.
