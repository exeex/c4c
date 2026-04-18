# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by moving the
short-circuit entry branch-target contract into shared prepare code. Shared
lowering now exposes one helper that validates the prepared compare-branch
handoff and returns the authoritative true/false target labels, and the Step 3
x86 local-slot short-circuit consumer now resolves its entry direct-branch
targets from that shared helper instead of revalidating the prepared branch
metadata inline.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local short-circuit composition seam, most likely by promoting the
remaining short-circuit branch-plan assembly or direct-target/continuation
composition into one shared prepared helper. Keep the work in shared consumer
helpers and focused proof, not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The new shared short-circuit entry-target helper only covers authoritative
  compare-branch label ownership; follow-on work should continue moving
  short-circuit branch/join composition into shared prepare code instead of
  rebuilding branch-plan or continuation routing in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared true/false ownership and continuation labels as
  authoritative; do not reintroduce raw selected-value plumbing or local join
  bundle reconstruction in the emitter.
- The joined-branch ownership tests still intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality checks in the x86 consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving the short-circuit entry
  branch-target contract into shared prepare code.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared short-circuit entry
branch-target helper, the x86 consumer that now uses it, and the existing
prepared branch/join ownership families that still prove the same handoff
contracts.
