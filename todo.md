# Current Packet

Status: Complete
Source Idea Path: ideas/open/526_bir_route5_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Route5 Publication Owner

## Just Finished

Step 2 completed for `ideas/open/526_bir_route5_publication_body_extraction.md`.
Added `src/backend/bir/bir_route5_publication.cpp` as the focused route5
publication owner and moved the mapped route5 publication bodies there without
changing route records, ordering, statuses, or route6 policy.

Moved route5 public bodies:

- `route5_publication_source_kind`
- `route5_cfg_edge_publication_record`
- `route5_current_block_join_source_records`
- `route5_edge_destination_value_record`
- `route5_edge_source_value_record`
- `route5_join_destination_value_record`
- `route5_join_source_value_record`
- `route5_build_edge_join_source_index`
- `route5_find_cfg_edge_publication`
- `route5_find_current_block_join_source`

Moved route5 private helpers:

- `route5_find_block_by_label`
- `route5_value_matches_record`

`src/backend/bir/bir.cpp` now retains the shared
`route4_record_matches_block` helper and the other route bodies. The new route5
owner uses the same narrow local forward declaration pattern for
`route4_record_matches_block` that `bir_route4_publication.cpp` already uses.
Public route5 declarations and record/enumeration types remain in
`src/backend/bir/bir.hpp`.

Updated `tests/backend/bir/CMakeLists.txt` only for the direct-source
`backend_lir_to_bir_notes_test` target so it compiles
`src/backend/bir/bir_route5_publication.cpp` alongside the existing route
sources. The backend library picked up the new source through the existing BIR
source glob.

## Suggested Next

Supervisor should select the next plan packet after reviewing the Step 2 slice
and proof. If the next packet continues extraction, keep it narrowly scoped to
the next mapped route owner and direct-source target wiring only.

## Watchouts

- `route4_record_matches_block` remains the shared non-route5 helper in
  `src/backend/bir/bir.cpp`; do not duplicate or rewrite its matching rule.
- `src/backend/bir/bir_route5_publication.cpp` depends on route1 and route3
  APIs directly; do not copy producer or memory-access logic into route5.
- Route6 policy was intentionally untouched for this packet.
- `backend_lir_to_bir_notes_test` is a direct-source target and must continue
  listing route source files explicitly when new BIR route translation units are
  needed by that test.

## Proof

Ran the supervisor-selected proof exactly:

`(cmake --build build -j --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test backend_riscv_prepared_edge_publication_test backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing|backend_riscv_prepared_edge_publication|backend_lir_to_bir_notes)$' --output-on-failure) > test_after.log 2>&1`

Result: passed. `test_after.log` shows `bir_route5_publication.cpp` compiled
for `c4c_backend` and `backend_lir_to_bir_notes_test`, then all four focused
tests passed:

- `backend_aarch64_current_block_join_routing`
- `backend_lir_to_bir_notes`
- `backend_prepared_lookup_helper`
- `backend_riscv_prepared_edge_publication`
