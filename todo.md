Status: Active
Source Idea Path: ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Edge Consumer Preservation Moves

# Current Packet

## Just Finished

Step 3 implemented the bounded RV64 materialization slice for coherent
predecessor-terminator
`edge_consumer_preservation_register_to_register` moves.

Implementation and evidence:

- `src/backend/mir/riscv/codegen/object_emission.cpp` now accepts unindexed
  register-to-register edge-consumer preservation moves in the matched
  out-of-SSA parallel-copy branch.
- The ordering policy is explicit: traverse `PreparedMoveBundle::moves` in
  producer-published order, emit unindexed preservation moves where they
  appear, and require each step-indexed `phi_join_register_to_register` move to
  consume the next `PreparedParallelCopyBundle::steps` entry. Every
  parallel-copy step must be consumed by the end of the move bundle.
- Accepted preservation moves require event kind `PreTerminatorCopies`, phase
  `BlockEntry`, authority `OutOfSsaParallelCopy`, matched
  `PredecessorTerminator` bundle, move reason
  `edge_consumer_preservation_register_to_register`, destination storage
  `Register`, op kind `Move`, no step index, matching prepared edge labels,
  and GPR source/destination homes.
- Step 2 phi materialization remains covered and unchanged.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` covers a
  preservation-before-phi sequence plus fail-closed preservation cases for
  unexpected step index, stack destination, stack-preservation reason, and edge
  coordinate confusion.
- `build/agent_state/502_step3_edge_preservation_register_moves/summary.md`
  records the implemented contract, ordering policy, and proof.

## Suggested Next

Execute the remaining idea 502 implementation packet for
`edge_consumer_preservation_register_to_stack` only. That packet should define
a stack-destination preservation policy using prepared homes/stack layout and
must keep the existing Step 2/Step 3 register behavior intact.

## Watchouts

- Do not infer preservation ordering from raw block order, source shape,
  filenames, labels, final homes, or target output; consume prepared
  move-bundle order plus parallel-copy steps.
- Keep critical-edge, successor-entry, save-destination-temp, cycle-temp,
  before-instruction, before-return, select-publication, producer repairs,
  expectation rewrites, unsupported marker changes, pass/fail accounting, and
  baseline churn outside the next packet unless explicitly delegated.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 3 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output is preserved in `test_after.log`.
