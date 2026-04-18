# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp` and
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by moving the
authoritative short-circuit branch-owned join-source classification and lookup
into shared prepared-control-flow helpers and switching the x86 short-circuit
consumer to use that shared contract. The emitter no longer keeps its own
duplicate short-circuit lane classification or branch-owned join-source wrapper
seam for this packet.

## Suggested Next

The next accepted packet should keep shrinking Step 3 emitter-local seams in
the same joined-branch family by lifting more compare-join continuation or
direct-branch planning checks out of `prepared_module_emit.cpp` and into
shared prepared consumer helpers where the contract is already authoritative.
Keep that work in semantic consumer helpers, not Step 4 file organization.

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
- The shared helpers in `prealloc.hpp` now own authoritative branch-owned
  join-transfer validation, supported SelectMaterialization / EdgeStoreSlot
  carrier queries, supported materialized compare-join predecessor and
  join-block shape validation, the prepared i32 eq/ne param-zero branch lookup
  used by both compare-consumer lanes, and the short-circuit incoming-lane
  classification / authoritative join-source lookup used by the short-circuit
  consumer. Follow-on work should keep extending prepared consumer lookups
  rather than reintroducing x86-side transfer-index, storage-name,
  predecessor, carrier-kind, predicate, param-zero operand, or short-circuit
  lane classification checks.
- The short-circuit ownership tests intentionally rewrite carrier labels and
  entry compares to prove x86 trusts prepared metadata over emitter-local
  carrier naming; do not add source-label equality checks that undercut that
  contract.
- A reverted proof-only packet tried to add nonzero trailing-join `add`
  coverage without any paired consumer change; treat more test-only `ne`
  trailing-op expansion as route drift unless the next slice also lands real
  Step 3 code.
- The joined-branch ownership helper still covers both
  `SelectMaterialization` and `PreparedJoinTransferKind::EdgeStoreSlot`
  carriers; keep further work in this family focused on prepared consumer
  seams rather than broader route expansion.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command for supervisor review.
- Treat any future fix here as capability repair, not expectation weakening:
  the eq/ne joined-branch and loop-countdown routes are covered by
  `backend_x86_handoff_boundary`.
- The joined-branch ownership tests now intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality requirements in the compare-join consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command after moving the short-circuit
  branch-owned join-source seam into shared prepared control-flow helpers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
This packet refreshes `test_after.log` with the same focused
`backend_x86_handoff_boundary` proof command for the shared short-circuit
join-source classification/lookup helper now consumed by the x86
short-circuit path.
