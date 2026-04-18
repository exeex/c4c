# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by extracting a shared
authoritative select-materialization join lookup for the materialized-compare
and short-circuit consumers, while keeping the short-circuit-only exact-two-lane
guard local so the joined-branch route does not lose broader authoritative
select-join coverage.

## Suggested Next

The next small Step 3 packet is to inspect whether
`find_authoritative_join_branch_sources()` can consume the already-validated
authoritative select-join context from the materialized-compare path, so x86
stops reclassifying the same prepared transfers while preserving the existing
topology and incoming-label checks.

## Watchouts

- Do not activate umbrella idea 57 or idea 59 while cleaning this helper
  surface.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth or new
  emitter-local CFG scans.
- The short-circuit path still intentionally differs from
  `find_authoritative_join_branch_sources()`: it consumes the authoritative
  prepared incoming ownership labels from the transfer records, but its
  continuation routing still depends on carrier labels that can differ from the
  prepared source ownership.
- The new shared authoritative-transfer helper only covers prepared transfer
  validation and source-label consistency; predecessor block lookup and
  branch-to-join topology checks still belong only to
  `find_authoritative_join_branch_sources()`.
- The short-circuit/select-join route still requires exactly one authoritative
  prepared source lane to carry a bool immediate; do not widen this into
  arbitrary edge scans or named-lane heuristics when touching adjacent helpers.
- If another consumer path needs extra branch or join facts, strengthen the
  shared prepared-control-flow contract in `prealloc.hpp` rather than growing
  emitter-local scans or CFG-shape recovery.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command for supervisor review.
- Keep the short-circuit path's `edge_transfers.size() == 2` requirement local
  to that route: the joined materialized-compare consumer still depends on
  authoritative select-join metadata without requiring the transfer list to
  collapse to exactly two total edges.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined-branch and loop-countdown routes are covered by
  `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
This Step 3 shared authoritative select-join lookup packet uses the same
focused proof command, and `test_after.log` is the canonical proof log path
for the result.
