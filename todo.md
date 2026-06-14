Status: Active
Source Idea Path: ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add explicit prepared compatibility proof where gaps exist

# Current Packet

## Just Finished

Step 2 - Added explicit prepared compatibility string-name proof in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` via
`verify_prepared_compatibility_status_names()`.

Newly protected exact prepared status names:
- `missing_prepared_memory_access`
- `incomplete_prepared_memory_access`
- `publication_unavailable` for prepared edge-copy source facts
- `unsupported_source_home`
- `missing_same_width_i32_type`
- `missing_destination_gpr_bank`
- `publication_unavailable` for prepared current-block-entry publication

The change is assertion-only. No status names, implementation paths, riscv
wrapper-visible output expectations, or route-native diagnostic rows were
changed.

## Suggested Next

Execute Step 3: add x86 Route 6 diagnostic naming beside retained compatibility
rows, keeping `ConsumedPlans`, prepared dump rows, and wrapper-output behavior
stable.

## Watchouts

Do not treat the new exact-name checks as replacement authority for route-native
diagnostics; they only pin retained prepared compatibility strings. Step 3
should add x86 Route 6 rows beside the existing compatibility surface, not
rename or remove prepared rows.

## Proof

Ran the delegated proof exactly:

`cmake --build build --target backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication)$' | tee test_after.log`

Result: passed 2/2:
`backend_prepared_lookup_helper` and
`backend_riscv_prepared_edge_publication`.

Proof log: `test_after.log`.
