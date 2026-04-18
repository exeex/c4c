Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- Plan Step: Step 2 / Step 3 - extended the shared short-circuit join contract
  so `PreparedJoinTransfer` records which originating branch-condition block
  produced a select-materialized join and which concrete join incoming label
  corresponds to the compare-true vs compare-false arms after linear branch
  collapse.
- Updated the bounded short-circuit x86 lane in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp` to use that prepared
  compare-truth mapping directly instead of walking empty CFG branch chains to
  rediscover which side short-circuits before the join.
- Added contract coverage in
  `tests/backend/backend_prepare_phi_materialize_test.cpp` and a prepared-x86
  ownership check in `tests/backend/backend_x86_handoff_boundary_test.cpp` that
  rewrites the prepared short-circuit join incoming labels to metadata-only
  names while keeping the authoritative prepared compare-truth mapping intact.

## Suggested Next
- Plan Step: Step 2 / Step 3 - push the bounded short-circuit lane further so
  x86 can take the join continuation path from prepared control-flow metadata
  without depending on the join block's remaining local carrier layout beyond
  the authoritative branch-condition record.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.
- The short-circuit slice now relies on the new
  `PreparedJoinTransfer.{source_branch_block_label,source_true_incoming_label,source_false_incoming_label}`
  mapping; new select-materialized join producers must populate those fields
  consistently or x86 will correctly reject the route.
- The bounded short-circuit lane still depends on the join block's prepared
  branch-condition record and continuation return leaves; if that remaining
  meaning needs to survive more carrier rewrites, extend the shared contract
  again instead of restoring CFG-shape recovery.
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
