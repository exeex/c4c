Status: Active
Source Idea Path: ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Phi Join Register Moves

# Current Packet

## Just Finished

Step 2 implemented RV64 materialization for the first coherent out-of-SSA
predecessor-terminator packet: step-indexed
`phi_join_register_to_register` register moves.

Implementation and evidence:

- `src/backend/mir/riscv/codegen/object_emission.cpp` now threads the matched
  `PreparedParallelCopyBundle` from prepared-object classification into move
  bundle fragment construction.
- A dedicated out-of-SSA branch emits only matched `PreTerminatorCopies` with
  phase `BlockEntry`, authority `OutOfSsaParallelCopy`, execution site
  `PredecessorTerminator`, reason `phi_join_register_to_register`,
  destination storage `Register`, op kind `Move`, present matching
  `source_parallel_copy_step_index`, and a plain non-cycle
  `PreparedParallelCopyStepKind::Move`.
- The branch resolves source and destination through prepared value homes and
  emits in `PreparedParallelCopyBundle::steps` order.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` covers the
  accepted `mv s1, t0` case and fail-closed adjacent shapes for
  edge-consumer preservation, stack destination, missing step index,
  save-destination-temp, cycle-temp source, and successor-entry execution.
- `build/agent_state/502_step2_phi_join_register_moves/summary.md` records the
  implemented contract and proof.

Select-publication parallel-copy routes were preserved: the new branch claims
only the step-indexed phi/edge-preservation family and lets existing
select-publication handling keep its prior specialized path.

## Suggested Next

Execute Step 3 as a separate packet for edge-consumer preservation register
moves only. That packet should define and implement the ordering/interleaving
policy for `edge_consumer_preservation_register_to_register` moves, which carry
out-of-SSA authority and edge labels but intentionally do not carry
`source_parallel_copy_step_index`.

## Watchouts

- Do not widen Step 3 into stack-destination preservation; the 3
  `edge_consumer_preservation_register_to_stack` rows still need a separate
  stack policy.
- Do not infer ordering from raw block order, source shape, filenames, labels,
  target output, or final homes. Consume prepared bundle/move authority.
- Keep critical-edge, successor-entry, save-destination-temp, cycle-temp,
  before-instruction, before-return, select-publication, producer repairs,
  expectation rewrites, unsupported marker changes, pass/fail accounting, and
  baseline churn outside the next packet unless explicitly delegated.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 2 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output is preserved in `test_after.log`.
