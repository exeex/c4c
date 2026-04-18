# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by adding a shared
prepared helper that publishes the authoritative same-module-global ownership
set for materialized compare-join return contexts, switching the x86
compare-join consumer to use that helper instead of accumulating global names
lane-by-lane in the emitter, and extending the handoff-boundary test suite to
prove the shared helper returns the expected ordered global set across direct,
fixed-offset, and pointer-backed compare-join cases.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join composition seam, most likely by moving one more
materialized compare-join return-lane render decision such as trailing
immediate-op ownership or return-context selection behind a shared prepared
consumer helper instead of open-coded lane-specific branching in
`prepared_module_emit.cpp`. Keep the work in shared consumer helpers and
focused proof, not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The new shared compare-join same-module-global helper only covers the ordered
  ownership set for selected-value global roots; x86 still spells the actual
  loads and emitted data, so follow-on work should keep moving authoritative
  compare/join composition into shared helpers instead of rebuilding lane
  ownership in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, or local join bundle reconstruction
  in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving compare-join same-module
  global ownership publication into shared prepare helpers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared compare-join
same-module-global ownership helper, the x86 consumer that now resolves it,
and the existing prepared branch/join ownership families that still prove the
same handoff contracts.
