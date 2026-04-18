# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp` and
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by moving the
param-zero materialized compare-join bundle composition into shared prepare
code. Shared lowering now exposes one helper that returns the prepared branch
contract plus the prepared compare-join branch bundle together, and the Step 3
x86 consumer now asks that single helper for authoritative branch/join facts
instead of composing those lookups locally before rendering the compare-join
lane.

## Suggested Next

The next accepted packet should stay in Step 3 and either remove another small
x86-local compare-join composition seam by promoting it into shared prepared
helper lookup, or escalate to a broader backend proving packet now that the
compare-join route has accumulated several narrow ownership and helper-bundle
slices. Keep the work in shared consumer helpers and focused proof, not Step 4
file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The new shared param-zero compare-join helper is only a composition seam over
  the existing prepared branch and prepared join helpers; follow-on work should
  continue moving composition into shared prepare code instead of rebuilding
  the same bundle shape in x86.
- The x86 compare-join branch renderer should continue treating the returned
  prepared true/false return contexts as authoritative; do not reintroduce raw
  selected-value plumbing or local join bundle reconstruction in the emitter.
- The joined-branch ownership tests still intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality checks in the x86 consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving the param-zero compare-join
  bundle lookup into shared prepare code.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the shared param-zero compare-join
bundle helper, the x86 consumer that now uses it, and the existing prepared
compare-join ownership families that still prove the same branch/join
contracts.
