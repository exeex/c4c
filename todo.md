# Current Packet

Status: Active
Source Idea Path: ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the result-level versus edge-bound baseline

## Just Finished

- Step 1 — added a registered prepared fact-boundary baseline with two
  independently valid edge-bound routing facts sharing one result-level key.
- The baseline proves the consumer key cannot distinguish the facts and names
  `predecessor_label` as the first lost identity, followed by destination
  identity, without changing backend selection or AArch64 integration behavior.

## Suggested Next

- Execute Step 2 by extracting separate registered probes for parallel
  predecessors, parallel destinations, wrong successor, and duplicate semantic
  edge outcomes.

## Watchouts

- Preserve the Step 1 distinction: result id/name, role, and current successor
  are insufficient once predecessor/destination diverge.
- Keep the existing AArch64 routing test as integration proof only.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; all 309/309 backend tests passed. The registered
  `backend_prepared_fact_boundary_contract` contains the focused baseline.
  Canonical proof log: `test_after.log`.
