Status: Active
Source Idea Path: ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Out-Of-SSA Parallel-Copy Surfaces

# Current Packet

## Just Finished

Step 1 inspected the prepared out-of-SSA parallel-copy publication and RV64
consumption surfaces without implementation changes.

Durable evidence:

- `build/agent_state/502_step1_out_of_ssa_parallel_copy_surfaces/summary.md`
- `build/agent_state/502_step1_out_of_ssa_parallel_copy_surfaces/representative_rows.tsv`

The idea 495 evidence splits the idea 502 family into 91
`pre_terminator_copies` rows with phase `block_entry`, authority
`out_of_ssa_parallel_copy`, `parallel_copy=yes`, and execution site
`predecessor_terminator`:

- 62 `phi_join_register_to_register` rows, represented by
  `src/20020619-1.c`.
- 26 `edge_consumer_preservation_register_to_register` rows, represented by
  `src/20020206-2.c`.
- 3 `edge_consumer_preservation_register_to_stack` rows, represented by
  `src/20041114-1.c`.

Prepared records/helpers to consume:

- `PreparedParallelCopyBundle` and `PreparedParallelCopyStep` for source edge
  labels, execution site, execution block label, ordered step indexes, and
  cycle metadata.
- `PreparedMoveBundle` and `PreparedMoveResolution` for route-local phase,
  out-of-SSA authority, execution block index, source edge labels, regalloc
  homes, destination storage, move reason, and optional
  `source_parallel_copy_step_index`.
- `find_prepared_out_of_ssa_parallel_copy_move_bundle` and
  `find_prepared_out_of_ssa_parallel_copy_move_for_step` for matching move
  bundles/moves to parallel-copy bundles.
- `classify_prepared_object_move_bundle_consumer` for the existing
  fail-closed validation that a traversal event's move bundle matches the
  parallel-copy bundle by authority, execution site, execution block, and edge
  labels.

RV64 hook to extend:

- The prepared-object event loop in
  `src/backend/mir/riscv/codegen/object_emission.cpp` already classifies
  `PreTerminatorCopies` with both a matched `move_bundle` and
  `parallel_copy_bundle`.
- `fragment_for_prepared_move_bundle` currently receives only the move bundle,
  so the next implementation should pass the matched parallel-copy bundle or
  full classification into a route-local out-of-SSA lowering helper before
  emitting register moves.

Current facts are complete enough for a narrow first implementation of
step-indexed predecessor-terminator `phi_join_register_to_register` moves. They
are not yet a single complete packet for all 91 rows because edge-consumer
preservation moves carry out-of-SSA authority and edge labels but no
`source_parallel_copy_step_index`, and the 3 stack-destination preservation
rows require separate stack materialization policy.

## Suggested Next

Execute Step 2 as a narrow implementation packet for only coherent
predecessor-terminator `phi_join_register_to_register` register moves:
`PreTerminatorCopies`, phase `BlockEntry`, authority `OutOfSsaParallelCopy`,
matched `PreparedParallelCopyBundle::PredecessorTerminator`, destination
storage `Register`, op kind `Move`, present `source_parallel_copy_step_index`,
plain non-cycle `PreparedParallelCopyStepKind::Move`. Keep edge-consumer
preservation, stack destinations, cycle temp, save-destination-temp,
successor-entry, critical-edge, before-instruction, before-return,
select-publication, producer repair, expectation rewrites, and baseline churn
out of Step 2.

## Watchouts

- Do not re-open before-instruction moves under idea 502.
- Do not implement before-return or select-publication handling under idea 502.
- Do not infer predecessor/successor, phi, edge, execution-site, destination
  storage, or parallel-copy ordering from raw block order, source shape,
  filenames, labels, final homes, or target output.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 1 proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed after the supervisor regenerated `test_before.log` as a backend
CTest log and reran the exact lifecycle proof command. Output is preserved in
`test_after.log`.
