# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet across
`src/backend/prealloc/prealloc.hpp` and
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by moving the
authoritative branch-owned join-transfer classification into a shared prepared
helper and switching the x86 materialized-compare and short-circuit join
consumers to use that shared lookup instead of reclassifying branch-owned join
transfers locally. The shared helper now validates the supported
`SelectMaterialization` and `PreparedJoinTransferKind::EdgeStoreSlot` contract
once, then hands x86 the authoritative join transfer plus its true/false edge
transfers as prepared data.

## Suggested Next

The next accepted packet should remove one more Step 3 emitter-local
branch-owned join seam only if it meaningfully changes prepared-data
consumption, not just file organization. If inspection shows the remaining
work is mostly x86 helper placement or extraction around
`prepared_module_emit.cpp`, hand lifecycle back for an explicit Step 4
transition instead of stretching Step 3.

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
- The shared helper in `prealloc.hpp` now owns authoritative branch-owned join
  transfer validation for the supported Step 3 contract, so follow-on work
  should extend that shared prepared surface rather than reintroducing x86-side
  transfer-index or storage-name checks.
- A reverted proof-only packet tried to add nonzero trailing-join `add`
  coverage without any paired consumer change; treat more test-only `ne`
  trailing-op expansion as route drift unless the next slice also lands real
  Step 3 code.
- The joined-branch ownership helper still covers both
  `SelectMaterialization` and `PreparedJoinTransferKind::EdgeStoreSlot`
  carriers; keep further work in this family focused on prepared consumer
  seams rather than broader route expansion.
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
shared authoritative branch-owned join helper plus the x86 consumers that now
use it.
