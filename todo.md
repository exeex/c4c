# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by broadening the
minimal compare-against-zero branch and joined-branch consumers from prepared
`eq i32 param, 0` only to prepared `eq/ne i32 param, 0` control-flow facts.
The x86 handoff path now derives false-branch polarity from prepared branch
metadata instead of hard-coding the equality-only lane, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` now proves actual
prepared `BinaryOpcode::Ne` ownership for both the plain branch and
branch-owned join cases.

## Suggested Next

Do not spend another packet on proof-only expansion of already-green `ne`
trailing-join cases. The next accepted packet must either remove one remaining
Step 3 emitter-local semantic-recovery seam in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` with a real prepared
branch/join consumer change, or hand lifecycle back for an explicit Step 4
transition if inspection shows the remaining gap is file organization rather
than consumer capability.

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
- A reverted proof-only packet tried to add nonzero trailing-join `add`
  coverage without any paired consumer change; treat more test-only `ne`
  trailing-op expansion as route drift unless the next slice also lands real
  Step 3 code.
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
prepared eq/ne branch-owned consumer surface.
