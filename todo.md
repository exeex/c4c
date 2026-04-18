Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- Plan Step: Step 2 / Step 3 - proved one loop-carried join slice through the
  shared prepared control-flow contract by checking that
  `PreparedBirModule.control_flow.join_transfers` publishes the loop-header
  `EdgeStoreSlot` incoming ownership together with the authoritative loop
  branch-condition metadata.
- Added a bounded phi-based countdown-loop x86 lane in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp` that consumes the
  prepared loop join and branch metadata to lower the loop in registers instead
  of recovering the loop-header join meaning from CFG topology or legalized
  store/compare carriers.
- Added shared-contract coverage in
  `tests/backend/backend_prepare_phi_materialize_test.cpp` and a prepared-x86
  ownership check in `tests/backend/backend_x86_handoff_boundary_test.cpp` that
  scrambles the legalized entry/body store carriers and loop compare carrier
  while keeping the authoritative prepared control-flow metadata intact.

## Suggested Next
- Plan Step: Step 2 / Step 3 - expand contract-driven x86 loop consumption past
  the single countdown decrement lane so another loop-carried join family can
  use prepared join/branch metadata without reintroducing CFG-shape recovery.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.
- The new loop slice is intentionally bounded to a single decrement-by-one
  countdown family whose backedge update still comes from the body arithmetic;
  broader loop forms should extend shared metadata before adding another x86
  shape-specific lane.
- The older short-circuit slice still relies on CFG traversal to locate which
  side reaches the join even though the branch predicate and join incoming
  semantics now come from shared metadata.
- If the next slice needs richer predecessor or carried-update identity than
  the current `EdgeStoreSlot` contract publishes, extend the shared contract
  instead of teaching x86 another whole-shape matcher.

## Proof
- Ran the delegated proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_x86_handoff_boundary(_test)?)$' | tee test_after.log`
- `test_after.log` captured the ctest portion of that proof run.
- Current result: delegated subset passed.
- Passing tests:
  `backend_prepare_phi_materialize`, `backend_x86_handoff_boundary`.
