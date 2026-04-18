# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by tightening the
materialized-compare join return seam to carry the validated carrier result
name through `find_materialized_compare_join_context()` and
`render_materialized_compare_join_return_if_supported()` instead of coupling
those checks to `join_transfer.result.name`. `backend_x86_handoff_boundary`
now mutates the joined-branch ownership fixture so the validated carrier result
intentionally diverges from the prepared join destination name, and it adds an
`EdgeStoreSlot` variant of that same route.

## Suggested Next

The next small Step 3 packet is to audit the remaining non-countdown
materialized-compare join route for any remaining consumer that still assumes
the prepared join destination name must match the validated carrier or trailing
join arithmetic operands, then tighten one such seam or declare this
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
- The materialized-compare join context now carries the validated carrier
  result name, so follow-on work should preserve that split rather than
  reintroducing checks against `join_transfer.result.name`.
- The joined-branch ownership helper now proves both `SelectMaterialization`
  and `PreparedJoinTransferKind::EdgeStoreSlot` carriers with a renamed
  carrier result; keep any further coverage in this family focused on prepared
  carrier validation rather than broader route expansion.
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
This materialized-compare join carrier-result packet passed with the focused
`backend_x86_handoff_boundary` proof command, and `test_after.log` remains the
fresh canonical proof log for supervisor review.
