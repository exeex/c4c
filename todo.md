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
resolved compare-join render contract so each packaged true/false return arm
now carries its authoritative resolved same-module-global ownership, switching
the x86 compare-join consumer to render those global-backed arms from the
resolved prepared contract instead of re-looking globals up by name in the
emitter, and extending the handoff-boundary test suite to prove that resolved
arm ownership survives both ordinary and `PreparedJoinTransferKind::EdgeStoreSlot`
compare-join carrier variants.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join consumption seam only if it still reflects shared
semantic ownership rather than x86 spelling, most likely by pushing one more
materialized compare-join return-arm decision or helper lookup out of
`prepared_module_emit.cpp` and into the shared prepared contract where the x86
consumer can keep treating packaged branch/join facts as authoritative. Keep
the work in shared consumer helpers and focused proof, not emitter cleanup or
broader backend route changes.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join render contract now carries authoritative branch
  labels, false-branch opcode, packaged true/false return arms, whole-contract
  same-module-global ownership, and per-arm resolved same-module-global
  references for the global-backed return lanes. Keep follow-on work focused
  on moving the remaining compare/join composition decisions into shared
  helpers instead of rebuilding lane semantics or global ownership in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, or local join bundle reconstruction
  in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving compare-join render-contract
  resolution into the shared helper path.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared resolved compare-join
arm contract, the x86 consumer that now renders global-backed compare-join
arms from the resolved prepared contract instead of local global-name lookup,
and the existing prepared branch/join ownership families that still prove the
same handoff contracts.
