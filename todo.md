Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- Plan Step: Step 2 / Step 3 - pushed the bounded short-circuit x86 consumer to
  seed its entry-side routing from the prepared branch-condition contract
  instead of the entry block's carrier `CondBranch` labels, keeping the
  short-circuit route authoritative on prepared control-flow metadata.
- Added a prepared-x86 ownership check in
  `tests/backend/backend_x86_handoff_boundary_test.cpp` that poisons both the
  entry and join carrier branch labels after preparation while leaving the
  prepared control-flow metadata intact; the route now still emits the same
  short-circuit asm from the shared contract.

## Suggested Next
- Plan Step: Step 3 - de-authorize the remaining shape-recovery fallback for
  this bounded short-circuit family when prepared control-flow metadata is
  present, so the lane fails closed instead of silently recovering through CFG
  shape if the shared contract regresses.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.
- The short-circuit slice now relies on
  `PreparedBranchCondition.{true_label,false_label}` plus
  `PreparedJoinTransfer.{source_branch_block_label,source_true_incoming_label,source_false_incoming_label}`;
  shared preparation must keep those records aligned or x86 will correctly
  reject the route.
- The bounded short-circuit lane still keeps a legacy shape-recovery fallback
  behind the contract-first path and still depends on real continuation leaf
  bodies; remove that fallback before treating this family as fully
  de-authorized from CFG recovery.
- Loop countdown support from the prior packet remains intentionally bounded to
  the decrement-by-one register lane; broader loop families still need shared
  metadata first.

## Proof
- Ran the delegated proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_x86_handoff_boundary(_test)?)$' | tee test_after.log`
- `test_after.log` captured the ctest portion of that proof run.
- Current result: delegated subset passed.
- Passing tests:
  `backend_prepare_phi_materialize`, `backend_x86_handoff_boundary`.
