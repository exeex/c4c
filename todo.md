# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by splitting the
authoritative branch/join lookup seam into a shared
`AuthoritativeJoinTransferSources` helper plus the stricter
`find_authoritative_join_branch_sources()` topology check, then making
`find_short_circuit_join_context()` consume that shared authoritative transfer
seam end-to-end without re-unwrapping raw prepared join transfers.

## Suggested Next

The next small Step 3 packet is to see whether the remaining short-circuit
bool-lane classification and `edge_transfers.size() == 2` gate can move behind
a dedicated authoritative short-circuit helper, so this route keeps consuming
prepared transfer facts through one seam without widening into countdown-loop
or generic join work.

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
- `find_branch_owned_join_transfer()` now returns null on ambiguity, so if a
  later route needs to distinguish multiple branch-owned join transfers from
  the same source block, strengthen the prepared lookup contract in
  `prealloc.hpp` instead of reintroducing emitter-local scans.
- `PreparedJoinTransferKind::EdgeStoreSlot` now flows through the joined
  materialized-compare consumer path only where the existing prepared
  authoritative transfer contract already applies; do not grow a new matcher
  family around it.
- The short-circuit path still intentionally differs from
  `find_authoritative_join_branch_sources()`: it consumes the authoritative
  transfer indices and prepared ownership metadata, but its continuation
  routing still depends on carrier labels that can differ from the prepared
  source ownership.
- `find_authoritative_join_branch_sources()` now expects an
  `AuthoritativeBranchJoinTransfer`; adjacent consumers should reuse that
  authoritative helper result instead of re-running raw transfer
  classification.
- `AuthoritativeJoinTransferSources` is intentionally weaker than
  `find_authoritative_join_branch_sources()`: keep source-label matching and
  direct predecessor topology checks in the joined-branch helper only, because
  the short-circuit route still permits carrier topology that does not look
  like a direct branch-to-join pair.
- Predecessor block lookup and branch-to-join topology checks still belong in
  x86 consumer code; do not push them into shared lowering while finishing this
  Step 3 seam.
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
This Step 3 short-circuit authoritative-transfer packet passed with the same
focused proof command after preserving the route's distinction between shared
authoritative transfer lookup and joined-branch-only topology checks, and
`test_after.log` is the canonical proof log path for the result.
