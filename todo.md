# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`tests/backend/backend_x86_handoff_boundary_test.cpp` by adding a concrete
nonzero trailing-join arithmetic fixture and route proof for the prepared
compare-join consumer. The x86 handoff lane now has end-to-end coverage for an
actual prepared `ne i32 param, 0` join-owning path with a trailing immediate
`add`, and the prepared-control-flow ownership checks keep proving that the
same consumer seam honors both direct prepared joins and
`PreparedJoinTransferKind::EdgeStoreSlot` carriers without reintroducing
emitter-local CFG recovery.

## Suggested Next

Pick one more small Step 3 proof packet that adds a concrete nonzero
trailing-join case for another already-supported immediate op such as `xor` or
`shl`, or stop and reassess whether the remaining gap is now route coverage
rather than consumer capability.

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
- Keep deriving branch polarity from prepared branch metadata rather than from
  equality-only assumptions in the emitter.
- The materialized-compare and short-circuit join helpers already share one
  prepared-carrier validation seam, so follow-on work should extend that
  seam instead of reintroducing per-helper carrier parsing or destination-name
  assumptions.
- The new nonzero trailing-join `add` coverage is intentionally one concrete
  proof packet; do not balloon the next slice into a wholesale duplication of
  every eq-only trailing op unless the supervisor explicitly wants broader
  route proof.
- The joined-branch ownership helper still proves both
  `SelectMaterialization` and `PreparedJoinTransferKind::EdgeStoreSlot`
  carriers; keep further work in this family focused on prepared carrier
  validation rather than broader route expansion.
- If another consumer path needs extra branch or join facts, strengthen the
  shared prepared-control-flow contract in `prealloc.hpp` rather than growing
  emitter-local scans or CFG-shape recovery.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command for supervisor review.
- Treat any future fix here as capability repair, not expectation weakening:
  the eq/ne joined-branch and loop-countdown routes are covered by
  `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
This packet passed with the same focused `backend_x86_handoff_boundary` proof
command, and `test_after.log` remains the fresh canonical narrow log for the
prepared eq/ne branch-owned consumer surface including the new concrete
nonzero trailing-join arithmetic lane.
