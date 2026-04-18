Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- Plan Step: Step 2 / Step 3 - extended shared legalization so
  `PreparedBirModule.control_flow.join_transfers` now records already-select-
  materialized short-circuit joins by pairing the authoritative branch-condition
  metadata with the linear predecessor labels that actually feed the join.
- Switched the bounded local-slot short-circuit `or` guard lane in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp` to prefer prepared
  branch/join metadata over the local compare/select carriers when choosing the
  short-circuit and continuation path.
- Added shared-contract coverage in
  `tests/backend/backend_prepare_phi_materialize_test.cpp` and a prepared-x86
  ownership check in `tests/backend/backend_x86_handoff_boundary_test.cpp` that
  desynchronizes the legalized short-circuit compare/select carriers while
  keeping the authoritative prepared control-flow metadata intact.

## Suggested Next
- Plan Step: Step 2 / Step 3 - take the same shared-contract route across one
  loop-carried join slice so x86 no longer has to recover the loop-header join
  semantics from CFG topology.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.
- The short-circuit packet still uses CFG traversal to find which side reaches
  the join, but the branch predicate and join incoming semantics now come from
  shared metadata rather than the local compare/select carrier instructions.
- If the next slice needs richer predecessor path identity than a linear branch
  chain, extend the shared contract instead of teaching x86 another whole-shape
  matcher.

## Proof
- Ran the delegated proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_x86_handoff_boundary(_test)?)$'`
- `test_after.log` captured the ctest portion of that proof run.
- Current result: delegated subset passed.
- Passing tests:
  `backend_prepare_phi_materialize`, `backend_x86_handoff_boundary`.
