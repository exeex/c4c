# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the prepared boundary inventory and status contract

## Just Finished

- Completed `plan.md` Step 1: added the narrow fail-closed
  `PreparedFactBoundaryStatus`/`PreparedFactBoundaryEvidence` contract and a
  public-header guard that freezes the inventoried route-shaped payloads.
- Inventory: `value_locations.hpp` has six Route 4 compatibility/proof names;
  `publication_plans.hpp` has six Route 5 compatibility/proof record, index,
  and field names. `module.hpp` additionally exposes `PrepareRoute` only as
  phase/debug metadata, while call preservation/address-materialization
  “route” vocabulary in `calls.hpp` is prepared-owned executable policy rather
  than a BIR route record or index.
- First owned producer seams are `publication_plans.cpp` publication
  production, then `call_plans.cpp` call attribution and
  `prepared_lookups.cpp` lookup attribution.

## Suggested Next

- Execute `plan.md` Step 2 as a bounded publication producer migration,
  beginning with one publication seam and keeping its positive and explicit
  negative named-input proof together.

## Watchouts

- Named BIR facts are input evidence only; prepared homes, moves, freshness,
  publication, call-plan, lookup, frame, and control decisions remain owned by
  prealloc.
- Do not create parallel authority or absorb the common MIR migration owned by
  idea 706.
- The public-header guard intentionally inventories legacy compatibility
  residue rather than blessing it; reduce its counts as producer seams migrate.

## Proof

- Passed after default-preset registration correction: `cmake --preset
  default && cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_'` (309/309 tests), including
  `backend_prepared_fact_boundary_contract`.
- Canonical proof log: `test_after.log`.
