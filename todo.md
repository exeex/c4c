# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3b.2
Current Step Title: Publish uniquely edge-bound prepared routing authority

## Just Finished

- Step 2.3b.2 — published pointer-free prepared routing facts bound to routing
  role, predecessor, successor, destination, source, routed value, and semantic
  publication origin after move/publication/freshness/evidence authority is
  available.
- Added a unique edge-bound selector: missing facts fail closed, same-value
  facts from another predecessor/successor/destination mismatch, and duplicate
  exact semantic facts are ambiguous. Prepared `JoinTransfer` uniqueness no
  longer depends on pointer identity.

## Suggested Next

- Execute Step 2.3b.3 to remove the Route 5 public compatibility payload and
  make the bounded AArch64 consumer read only these selected prepared routing
  facts.

## Watchouts

- Step 2.3b.3 must query the edge-bound routing facts; do not recreate
  function-wide publication/`JoinTransfer` scans or target-generated evidence.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; all 309/309 backend tests passed. The registered
  `backend_prepared_fact_boundary_contract` covers parallel same-source, wrong
  successor, wrong destination, duplicate semantic routing, and the Step
  2.3b.1 mismatched named-producer boundary. Canonical proof log:
  `test_after.log`.
