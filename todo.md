# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3b.3
Current Step Title: Retire the bounded Route 5 compatibility consumer

## Just Finished

- Completed Step 2.3b.3: removed the Route 5 record/status/agreement payload
  and index input from the public current-block publication query, deleted its
  construction and transport, and retired printer/test agreement contracts.
- Confirmed the bounded AArch64 consumer remains directly attached to the
  unique prealloc-owned stable-key routing consumption query.

## Suggested Next

- Execute Step 2.3c: audit and prove publication-family closure across the
  completed block-entry, edge, and current-block contractions.

## Watchouts

- Common MIR Route 5 query/index coverage remains intentionally separate from
  the retired prepared compatibility payload.
- Preserve the owner-attached AArch64 query and its fail-closed stable-key
  ambiguity behavior during the Step 2.3c integration audit.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`.
- Result: 329/329 backend tests passed; canonical proof log is
  `test_after.log`.
