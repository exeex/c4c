# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate prepared call-plan production

## Just Finished

- Completed Step 3: prepared call-plan production now consumes
  `BirCallBoundaryView` plus named argument-source facts without mutating BIR
  to backfill Route 6 relationships.
- Preserved prealloc ownership of argument/result homes, ABI lanes/resources,
  moves, and materialization while rejecting unavailable, incomplete,
  ambiguous, and mismatched call/value evidence.

## Suggested Next

- Execute Step 4: migrate prepared lookup attribution to named BIR facts while
  preserving prepared lookup, home, and freshness authority.

## Watchouts

- Aggregate routing metadata remains a BIR semantic input for identifying
  grouped carriers; all ABI placement and materialization choices remain in
  prealloc.
- Keep the new call-plan boundary fail-closed; do not restore relationship
  synthesis or route-discovery fallback in later lookup work.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`.
- Result: 329/329 backend tests passed, including focused available direct
  call/result and missing, incomplete, ambiguous, and mismatched-value call
  boundary coverage; canonical proof log is `test_after.log`.
