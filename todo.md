# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by making the
materialized compare-join consumer validate `PreparedJoinTransferKind::EdgeStoreSlot`
through the prepared join contract instead of accepting any same-named
`LoadLocal` carrier. The authoritative branch-owned join helper now requires
the prepared slot metadata on `EdgeStoreSlot` transfers, and the compare-join
context only accepts a `LoadLocal` carrier when it names the prepared storage
slot published by that join transfer.

## Suggested Next

The next small Step 3 packet is to carry the same authoritative join-slot
contract into the next branch-owned `LoadLocal` consumer path that still keys
off the joined value name alone, while keeping short-circuit topology rules,
countdown-loop handling, and Step 4 emitter cleanup out of scope.

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
- The compare-join route now treats `EdgeStoreSlot` as an explicit prepared
  slot contract: if a future consumer accepts a `LoadLocal` carrier, it should
  validate the prepared storage name instead of matching only the joined value
  result.
- `find_materialized_compare_join_context()` is still intentionally scoped to
  the joined compare-return route: it validates the authoritative prepared join
  carrier and trailing binary around the prepared join result, but it should
  not absorb the short-circuit route's continuation/topology rules.
- `render_materialized_compare_join_branches_if_supported()` is the current
  prepared join-consumer seam for this route. Keep follow-on work keyed by the
  prepared join transfer and edge metadata instead of rebuilding transfer
  meaning from join-block scans or local instruction names.
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
This `EdgeStoreSlot` consumer packet passed with the focused
`backend_x86_handoff_boundary` proof command, and `test_after.log` remains the
fresh canonical proof log for supervisor review.
