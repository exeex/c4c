# Current Packet

Status: Active
Source Idea Path: ideas/open/528_bir_route6_call_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Includes And Internal Linkage

## Just Finished

Step 2: Create Route6 Call Publication Owner.

Moved only the mapped route6 call-use, call-argument publication, call-result,
and publication-routing bodies out of `src/backend/bir/bir.cpp` into
`src/backend/bir/bir_route6_call_publication.cpp`.

Public route6 declarations and types remained in `bir.hpp`. Route-index facade
bodies and facade-backed route7/materialized-condition helpers remained in
`bir.cpp`. Memory provenance/lowering surfaces, route1-route5 owner bodies,
call ABI lowering, LIR-to-BIR call generation, and idea 422 producer behavior
were not edited.

Route6 continues to consume route1-route5 behavior through existing public APIs.
The generic `indexed_call_inst` helper stayed in `bir.cpp` and was declared in
`bir_private.hpp` for the new route6 owner. Core backend build registration is
covered by the existing `src/backend/CMakeLists.txt` BIR source glob; the
standalone `backend_lir_to_bir_notes_test` source list in
`tests/backend/bir/CMakeLists.txt` now includes
`bir_route6_call_publication.cpp`.

## Suggested Next

Supervisor can delegate Step 3 from `plan.md`: verify/repair includes and
internal linkage after the route6 owner split. The Step 2 backend proof already
passed with the moved owner linked.

## Watchouts

- Keep this activation aligned to
  `ideas/open/528_bir_route6_call_publication_body_extraction.md`.
- Do not move route6 public declarations, route-index facade bodies, memory
  provenance headers, or implementation bodies outside the route6
  call-publication scope in the first mapping packet.
- Do not change call ABI lowering, LIR-to-BIR call generation, route1-route5
  behavior, or idea 422 producer capability.
- `src/backend/CMakeLists.txt` uses `GLOB_RECURSE` for `src/backend/bir/*.cpp`,
  so no core backend registration edit was needed.

## Proof

Ran exactly:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
