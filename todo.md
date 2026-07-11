# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Contract block-entry, edge, and current-block publication state

## Just Finished

- Step 2.2 — classified formal publication origins as incoming ABI or
  fixed-formal store-source composition. Applicable fixed-formal named BIR
  producer evidence must now be complete, unique, and match producer kind,
  block, instruction, and value identity before formal publication is
  available.
- Incoming ABI formals retain prepared-owned home, ABI, frame, and move
  authority, while fixed-formal composition reuses the Step 2.1 store-source
  evidence boundary and fails closed for missing, incomplete, ambiguous, or
  mismatched evidence.

## Suggested Next

- Supervisor review Step 2.2 as complete and select the Step 2.3 publication
  state contraction packet.

## Watchouts

- Keep the fixed-formal evidence applicability list aligned with the
  store-source boundary; do not make incoming ABI publication require a
  same-block producer because its semantic origin is not a BIR instruction.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; all 309/309 backend tests passed. Canonical proof log:
  `test_after.log`.
