# Current Packet

Status: Complete
Source Idea Path: ideas/open/526_bir_route5_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Includes And Internal Linkage

## Just Finished

Step 3 completed for `ideas/open/526_bir_route5_publication_body_extraction.md`.
Verified there is no remaining route5 include or internal-linkage fallout after
the body move.

No additional include/linkage repair was needed beyond the new owner source and
direct-source test wiring already committed in Step 2:

- `src/backend/bir/bir_route5_publication.cpp` exists as the focused route5
  publication owner.
- Public route5 declarations remain in `src/backend/bir/bir.hpp`.
- `route4_record_matches_block` remains owned by `src/backend/bir/bir.cpp` and
  route5 uses the same narrow local forward declaration pattern as route4.
- `backend_lir_to_bir_notes_test` directly lists
  `src/backend/bir/bir_route5_publication.cpp`; the backend library source glob
  also picked up the new owner source during the Step 2 build.

## Suggested Next

Supervisor should select the next plan packet. If the next packet continues
extraction, keep it narrowly scoped to the next mapped route owner and any
required direct-source target wiring.

## Watchouts

- `route4_record_matches_block` remains the shared non-route5 helper in
  `src/backend/bir/bir.cpp`; do not duplicate or rewrite its matching rule.
- `backend_lir_to_bir_notes_test` is a direct-source target and must continue
  listing route source files explicitly when new BIR route translation units are
  needed by that test.

## Proof

No rerun was required for this verification packet. Used the existing green
Step 2 proof in `test_before.log`.

Step 2 proof command:

`(cmake --build build -j --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test backend_riscv_prepared_edge_publication_test backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing|backend_riscv_prepared_edge_publication|backend_lir_to_bir_notes)$' --output-on-failure) > test_after.log 2>&1`

Result: passed. `test_before.log` shows `bir_route5_publication.cpp` compiled
for `c4c_backend` and `backend_lir_to_bir_notes_test`, then all four focused
tests passed:

- `backend_aarch64_current_block_join_routing`
- `backend_lir_to_bir_notes`
- `backend_prepared_lookup_helper`
- `backend_riscv_prepared_edge_publication`
