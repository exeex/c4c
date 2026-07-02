# Out-Of-SSA Parallel-Copy Move-Bundle Publication

Status: Open
Type: Producer/fact-propagation repair
Parent: `ideas/closed/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`
Owning Layer: Prepared value-location move-bundle publication

## Goal

Repair the producer or fact-propagation path that should publish
out-of-SSA parallel-copy move bundles for prepared traversal consumers.

## Why This Exists

The evidence-gap route for `src/960209-1.c` is complete. The row no longer
lacks first-owner evidence: prepared traversal has a
`pre_terminator_copies` event for prepared block label 20 and the parallel-copy
edge 20 -> 19, but value locations do not expose a matching
out-of-SSA parallel-copy move bundle for execution block label 20 / block index
15 and predecessor label 20.

Fresh evidence:

- `prepared_consumer_category=missing_move_bundle`
- `event_kind=pre_terminator_copies`
- `event_block_index=15`
- `prepared_block_label=20`
- `parallel_copy_predecessor=20`
- `parallel_copy_successor=19`
- `parallel_copy_execution_block=20`
- `lookup_execution_block_index=15`
- `candidate_move_bundle_count=21`
- `candidate_phase_block_entry_count=3`
- `candidate_authority_out_of_ssa_parallel_copy_count=3`
- `candidate_execution_block_count=0`
- `candidate_predecessor_label_count=0`
- `candidate_successor_label_count=1`
- `candidate_exact_parallel_copy_match_count=0`
- `value_home_type_f128_facts=unavailable_at_missing_move_bundle`

The useful next owner is therefore not RV64 materialization, F128 quarantine, or
diagnostic printing. It is the publication path that decides which execution
block, predecessor, and successor coordinates are attached to out-of-SSA
parallel-copy move bundles.

## In Scope

- Trace where out-of-SSA parallel-copy move bundles are produced, keyed, and
  attached to prepared value-location facts.
- Repair generalized coordinate propagation for execution block,
  predecessor, and successor labels so prepared traversal can find the move
  bundle that matches the event it is consuming.
- Preserve the existing diagnostic evidence path so missing-publication
  failures remain auditable.
- Add or update focused backend tests that prove the producer publishes a
  matching out-of-SSA parallel-copy move bundle without relying on
  `src/960209-1.c` by name.
- Use the one-row `src/960209-1.c` focused scan as proof that this failure
  moved past `missing_move_bundle` or was narrowed to a deeper semantic
  blocker.

## Out Of Scope

- RV64 materialization of the eventual move sequence after the move bundle is
  found.
- F128 helper, ABI, or quarantine work unless fresh row-level evidence proves
  this route has become F128-primary.
- Prepared consumer workarounds that infer a bundle from the event instead of
  consuming published producer facts.
- Expectation rewrites, unsupported marker edits, allowlist filtering, or
  runtime comparison changes.
- Filename-specific handling for `src/960209-1.c`.

## Acceptance Criteria

- Prepared traversal finds a matching out-of-SSA parallel-copy move bundle for
  the event coordinates, or the row advances to a different auditable first
  blocker after the publication repair.
- The repair is keyed by semantic producer facts such as phase, authority,
  execution block, predecessor, and successor labels, not by testcase shape.
- Focused backend contract/unit coverage proves the coordinate publication
  behavior.
- The one-row RV64 gcc torture backend scan for `src/960209-1.c` no longer
  reports `prepared_consumer_category=missing_move_bundle` for the same event,
  unless a new diagnostic demonstrates a deeper producer-owned blocker.
- No gcc_torture expectations, unsupported markers, allowlists, or runtime
  comparison behavior are weakened.

## Reviewer Reject Signals

- Reject fixes that special-case `src/960209-1.c`, block label 20, predecessor
  20, successor 19, or block index 15 instead of repairing the general
  out-of-SSA parallel-copy publication rule.
- Reject prepared consumer inference that fabricates a move bundle when the
  value-location producer still does not publish one.
- Reject RV64 materialization changes claimed as progress while the prepared
  consumer still reports `missing_move_bundle` for the same event.
- Reject helper renames, diagnostic wording changes, or classification-only
  updates claimed as capability repair.
- Reject unsupported downgrades, expectation rewrites, allowlist filtering, or
  weaker runtime comparisons as progress.
- Reject broad value-location or prepared traversal rewrites that are not tied
  to the audited phase/authority/execution-block/predecessor/successor
  coordinate mismatch.
