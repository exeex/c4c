# Current Packet

Status: Active
Source Idea Path: ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define the prealloc-owned result-consumption query

## Just Finished

- Step 3 — added a pointer-free prealloc-owned current-block routing-consumption
  query keyed by successor, routed value identity, and requested routing role.
- The query returns `Available` only when every applicable distinct edge fact
  has one invariant publication-semantic origin; it preserves explicit
  `Missing`, `Mismatched`, and `Ambiguous` outcomes, including duplicate edges.
- All four Step 2 probes now directly exercise the result-consumption query:
  parallel predecessors and destinations aggregate, wrong successor mismatches,
  and duplicate semantic edges remain ambiguous.

## Suggested Next

- Execute Step 4 by transporting the stable result-consumption key into the
  bounded AArch64 consumer and consuming only the prealloc-owned query result.

## Watchouts

- Keep duplicate semantic-edge detection ahead of aggregation; distinct edges
  may aggregate only when their publication-semantic origin is invariant.
- Step 4 must not reconstruct this authority with target-side scans, and should
  leave Route 5 public payload retirement for the later idea-705 handoff.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; all 313/313 backend tests passed. The four independently
  registered Step 2 probes directly passed against the Step 3 consumption
  query. Canonical proof log: `test_after.log`.
