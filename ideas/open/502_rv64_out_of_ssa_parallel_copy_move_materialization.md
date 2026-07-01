# RV64 Out-Of-SSA Parallel-Copy Move Materialization

Status: Open
Type: Focused RV64 materialization idea
Parent: `ideas/open/495_prepared_move_bundle_materialization_bucket_review.md`
Source Evidence:
- `build/agent_state/495_step2_move_bundle_coherence/summary.md`
- `build/agent_state/495_step2_move_bundle_coherence/classification.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv`
Owning Layer: RV64 object lowering for coherent prepared out-of-SSA parallel copies

## Goal

Materialize coherent prepared out-of-SSA pre-terminator parallel-copy move
bundles in RV64 while preserving predecessor-edge execution semantics.

## Why This Exists

Idea 495 Step 2 classified 91 rows as coherent prepared-authority
`pre_terminator_copies` with `authority=out_of_ssa_parallel_copy`,
`parallel_copy=yes`, and `execution_site=predecessor_terminator`:

- 62 register-destination rows for `phi_join_register_to_register`;
- 26 register-destination rows for
  `edge_consumer_preservation_register_to_register`;
- 3 stack-slot destination rows for
  `edge_consumer_preservation_register_to_stack`.

Representative rows include `src/20020619-1.c`, `src/20020206-2.c`, and
`src/20041114-1.c`.

## In Scope

- Consume prepared out-of-SSA parallel-copy records at predecessor terminators.
- Lower register-to-register and register-to-stack edge moves without changing
  the prepared edge execution site.
- Preserve parallel-copy semantics, including cycle/order safety, from the
  prepared bundle representation.
- Add focused backend coverage for phi-join and edge-consumer preservation
  representatives plus fail-closed incomplete-authority boundaries.

## Out Of Scope

- Before-instruction consumer moves.
- The single before-return move shape.
- Select-publication evidence or authority repair.
- Inventing predecessor/successor, phi, or edge identity in RV64.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- RV64 lowers coherent out-of-SSA parallel-copy move bundles only from prepared
  authority records.
- Representative phi-join and edge-consumer preservation rows no longer fail
  for `unsupported_move_bundle_target_shape` for this owner family.
- Missing edge identity, missing execution site, incomplete parallel-copy
  facts, or contradictory storage remain unavailable/fail-closed.
- Focused backend proof covers register-to-register and register-to-stack
  pre-terminator materialization.

## Reviewer Reject Signals

- Reject branch-edge inference from raw block order, source shape, filenames,
  labels, or target object output.
- Reject serializing a parallel copy in a way that can clobber a live source
  without prepared cycle/order handling.
- Reject combining this idea with before-instruction moves, before-return
  moves, select-publication evidence, or producer repairs.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as progress.
