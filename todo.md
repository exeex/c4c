Status: Active
Source Idea Path: ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Edge Consumer Preservation Moves

# Current Packet

## Just Finished

Step 3 completed the remaining bounded RV64 materialization slice for coherent
predecessor-terminator
`edge_consumer_preservation_register_to_stack` moves.

Implementation and evidence:

- `src/backend/mir/riscv/codegen/object_emission.cpp` now accepts stack
  destination edge-consumer preservation moves in the matched out-of-SSA
  parallel-copy branch.
- Accepted stack-preservation moves require event kind `PreTerminatorCopies`,
  phase `BlockEntry`, authority `OutOfSsaParallelCopy`, matched
  `PredecessorTerminator` bundle, reason
  `edge_consumer_preservation_register_to_stack`, destination storage
  `StackSlot`, op kind `Move`, no step index, matching prepared edge labels,
  a prepared GPR source home, and a prepared scalar stack-slot destination
  home/layout.
- The store policy uses producer-published `PreparedMoveBundle::moves` order,
  resolves the destination with `prepared_stack_slot_home_offset`, derives a
  scalar store size from BIR type when present or prepared stack-home size
  otherwise, and emits `append_rv64_store_register_to_stack`.
- Step 2 phi materialization and the previous Step 3
  `edge_consumer_preservation_register_to_register` behavior remain covered.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` covers the
  accepted stack-preservation sequence and fail-closed cases for missing stack
  layout, non-GPR source, unexpected step index, and stack/register coordinate
  confusion.
- `build/agent_state/502_step3_edge_preservation_stack_moves/summary.md`
  records the implemented contract, store policy, and proof.

## Suggested Next

Execute Step 4 as a residual disposition packet for idea 502. Record whether
out-of-SSA/pre-terminator parallel-copy materialization is complete enough for
downstream RV64 gcc-torture admission reclassification, or name the exact
remaining lower blocker if one remains.

## Watchouts

- Keep critical-edge, successor-entry, save-destination-temp, cycle-temp,
  before-instruction, before-return, select-publication, producer repairs,
  expectation rewrites, unsupported marker changes, pass/fail accounting, and
  baseline churn outside the residual packet unless explicitly delegated.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 3 stack-preservation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output is preserved in `test_after.log`.
