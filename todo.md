# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by adding a direct shared
helper that goes from the param-zero materialized compare-join packet inputs to
the resolved render contract, switching the x86 compare-join emitter to consume
that helper instead of composing the param-zero branch lookup and resolved
contract lookup locally, and extending the handoff-boundary test suite to prove
the new direct helper preserves the authoritative compare-join branch plan,
resolved globals, resolved arm ownership, and return-arm binary-plan context.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join consumption seam only if it still reflects shared
semantic ownership rather than x86 spelling, most likely by pushing one more
materialized compare-join consumption decision out of
`prepared_module_emit.cpp` and into a shared prepared helper so the x86
consumer keeps treating param-zero compare-join lane data as authoritative.
Keep the work focused on one helper-sized seam, not emitter cleanup or broader
backend route changes.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join render contract now has both the existing
  branch-packet-to-resolved-contract helper and the new direct param-zero
  resolved-contract helper. Keep follow-on work focused on moving remaining
  compare/join composition decisions into shared helpers instead of rebuilding
  lane semantics, label ownership, or global ownership in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, local branch-packet composition, or
  local join bundle reconstruction in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving the param-zero compare-join
  resolved render-contract lookup into the shared helper path.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared direct param-zero
compare-join resolved render-contract helper, the x86 consumer that now uses
that authoritative helper instead of composing the lookup locally, and the
existing prepared branch/join ownership families that still prove the same
handoff contracts. The delegated proof passed and `test_after.log` is the
preserved proof log.
