Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished
- Plan Step: Step 1 - added explicit prepared control-flow contract surfaces in
  `PreparedBirModule.control_flow` for branch conditions and join transfers.
- Legalize now publishes compare-backed branch metadata plus join-transfer
  records for select-materialized and slot-backed phi lowering routes.
- Added proof coverage in `tests/backend/backend_prepare_phi_materialize_test.cpp`
  to assert the new shared contract for the reducible join slice.

## Suggested Next
- Plan Step: Step 2 - extend the shared control-flow contract across
  short-circuit and loop-adjacent slices, then switch one bounded x86 consumer
  path in `src/backend/mir/x86/codegen/prepared_module_emit.cpp` to read
  `PreparedBirModule.control_flow` instead of reconstructing branch/join
  semantics from CFG shape.

## Watchouts
- Do not treat x86 matcher widening as progress for this idea.
- Keep semantic transfer ownership separate from idea `60` physical location
  ownership.

## Proof
- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_prepare_phi_materialize$' --output-on-failure`
