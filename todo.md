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
compare-join return-arm binary-plan helper that publishes whether a packaged
return lane owns a trailing immediate op, switching the x86 compare-join
consumer to use that authoritative helper instead of re-deciding trailing-op
ownership from arm shapes in `prepared_module_emit.cpp`, and extending the
handoff-boundary test suite to prove the render contract still packages that
trailing compare-join ownership correctly.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join consumption seam only if it still reflects shared
semantic ownership rather than x86 spelling, most likely by pushing one more
materialized compare-join return-arm helper lookup out of
`prepared_module_emit.cpp` and into the shared prepared contract so the x86
consumer keeps treating packaged compare-join lane data as authoritative. Keep
the work focused on one helper-sized seam, not emitter cleanup or broader
backend route changes.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join render contract now carries authoritative branch
  labels, false-branch opcode, packaged true/false return arms, whole-contract
  same-module-global ownership, per-arm resolved same-module-global references
  for the global-backed return lanes, and the packaged trailing-immediate
  binary plan for each compare-join return arm. Keep follow-on work focused on
  moving the remaining compare/join composition decisions into shared helpers
  instead of rebuilding lane semantics, trailing-op ownership, or global
  ownership in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, shape-based trailing-op decisions, or
  local join bundle reconstruction in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving compare-join return-arm
  trailing-op ownership into the shared helper path.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared compare-join
return-arm binary-plan helper, the x86 consumer that now renders trailing
compare-join immediate ops from that authoritative helper instead of local
shape switches, and the existing prepared branch/join ownership families that
still prove the same handoff contracts.
