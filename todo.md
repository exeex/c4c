# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by extending the shared
prepared compare-join render contract with authoritative true/false return-lane
shape classification, switching the x86 consumer to use those shared lane
shapes instead of deciding locally whether each compare-join lane is direct or
trailing-immediate, and extending the handoff-boundary test suite to prove the
render contract still packages the authoritative branch plan, return contexts,
return-lane shapes, and same-module-global ownership for this materialized
compare-join family.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join composition seam by moving one more materialized
compare-join consumer seam into shared helpers, most likely packaging one more
authoritative selected-value family or same-module-global emission decision so
`prepared_module_emit.cpp` keeps shrinking its compare-join-specific control
logic without drifting into Step 4 file organization. Keep the work in shared
consumer helpers and focused proof, not emitter cleanup.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The new shared compare-join render contract now packages the authoritative
  branch labels, false-branch opcode, true/false return contexts, true/false
  return-lane shapes, and same-module-global ownership set, but x86 still owns
  target-specific spelling and support checks for those packaged lane forms.
  Follow-on work should keep moving authoritative compare/join composition into
  shared helpers instead of rebuilding lane semantics in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, or local join bundle reconstruction
  in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving compare-join return-lane
  shape classification into the shared render contract helper.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared compare-join
render-contract helper, the x86 consumer that now resolves it, and the
existing prepared branch/join ownership families that still prove the same
handoff contracts.
