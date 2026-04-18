# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by tightening the
short-circuit join continuation seam to key the authoritative compare contract
off the validated carrier result instead of `join_transfer.result.name`. The
consumer now accepts `SelectMaterialization` and `PreparedJoinTransferKind::
EdgeStoreSlot` carriers through the prepared carrier it validated, and
`backend_x86_handoff_boundary` now mutates both short-circuit fixtures so the
carrier result name intentionally diverges from the prepared join destination
name.

## Suggested Next

The next small Step 3 packet is to audit the remaining non-countdown
materialized-compare join return path in `find_materialized_compare_join_context()`
and `render_materialized_compare_join_return_if_supported()` for any place
that still keys acceptance off `join_transfer.result.name` instead of the
validated carrier result, then tighten one such seam or declare this
carrier-name sub-route exhausted.

## Watchouts

- Do not activate umbrella idea 57 or idea 59 while cleaning this helper
  surface.
- Do not widen this Step 3 packet into generic instruction selection.
- Do not pull in idea 60 value-location work or idea 61 frame/addressing work.
- Do not touch countdown-loop routes in this packet.
- Do not pre-spend Step 4 by turning this into `prepared_module_emit.cpp`
  file-organization cleanup.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth or new
  emitter-local CFG scans.
- The short-circuit continuation helper now trusts the validated carrier result
  name for both `SelectMaterialization` and `EdgeStoreSlot`, so any follow-on
  packet should preserve that producer/consumer split instead of drifting back
  to `join_transfer.result.name` checks.
- `find_materialized_compare_join_context()` is still the remaining Step 3 seam
  most likely to retain the old join-result-name coupling; keep any follow-on
  work bounded there instead of widening into broader emitter cleanup.
- If another consumer path needs extra branch or join facts, strengthen the
  shared prepared-control-flow contract in `prealloc.hpp` rather than growing
  emitter-local scans or CFG-shape recovery.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command for supervisor review.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined-branch and loop-countdown routes are covered by
  `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
This short-circuit carrier-result packet passed with the focused
`backend_x86_handoff_boundary` proof command, and `test_after.log` remains the
fresh canonical proof log for supervisor review.
