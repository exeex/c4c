# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3c
Current Step Title: Integrate and prove publication-family closure

## Just Finished

- Completed Step 2.3c: audited the public block-entry, edge, and current-block
  publication payloads and selection seams; no Route 4/5 record, status,
  index, agreement predicate, fallback, or synthesized BIR publication
  relation remains in the migrated public publication boundary.
- Confirmed BIR-originated publication paths remain named-evidence-bound and
  fail closed, while prepared-originated `JoinTransfer` paths retain complete,
  unique prepared authority plus applicable producer identity.

## Suggested Next

- Execute Step 3: migrate prepared call-plan production to named BIR
  call-boundary and producer facts while retaining prepared ABI authority.

## Watchouts

- Common MIR Route 4/5 query/index implementation remains intentionally
  separate from the closed prepared publication payload.
- Preserve the completed publication-family boundary while Step 3 removes
  Route 6 inputs from prepared call-plan attribution.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`.
- Result: 329/329 backend tests passed; canonical proof log is
  `test_after.log`.
