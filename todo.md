# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp` and
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by promoting the
materialized compare-join predecessor, join-block, and return-shape validation
into a shared prepared-control-flow helper and switching the x86 compare-join
consumer to use that prepared context directly. The joined-branch carrier and
join-shape checks now sit in the same shared contract, so x86 no longer keeps
its own copy of that authoritative compare-join lookup logic.

## Suggested Next

The next accepted packet should keep shrinking Step 3 emitter-local seams in
the same joined-branch family by lifting the remaining prepared param-zero
branch predicate/value validation seam out of `prepared_module_emit.cpp` so
the minimal compare-branch and materialized compare-join lanes share a single
prepared branch-consumer helper. Keep that packet in semantic consumer
helpers, not file organization.

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
- The shared helpers in `prealloc.hpp` now own both authoritative
  branch-owned join-transfer validation and the supported SelectMaterialization
  / EdgeStoreSlot carrier query plus the supported materialized compare-join
  predecessor and join-block shape validation, so follow-on work should keep
  extending prepared consumer lookups rather than reintroducing x86-side
  transfer-index, storage-name, predecessor, or carrier-kind checks.
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

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
This packet refreshes `test_after.log` with the same focused
`backend_x86_handoff_boundary` proof command for the compare-join consumer
path after moving the remaining predecessor/join-shape validation into shared
prepared-control-flow helpers.
