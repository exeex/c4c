Status: Active
Source Idea Path: ideas/open/132_aarch64_dispatch_lookup_thin_helper_surface_trim.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Privatize Or Remove Proven Local Lookup Helpers

# Current Packet

## Just Finished

Step 2: Privatize Or Remove Proven Local Lookup Helpers completed for `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp` and `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`.

Removed the proven no-direct-caller public lookup declarations:

- `is_scalar_call_argument_producer_opcode`
- `find_same_block_scalar_producer`
- `has_same_block_load_local_producer`

Removed the matching dead definitions from `dispatch_lookup.cpp`. Removing `find_same_block_scalar_producer` also made the private `prepared_call_argument_source_producer_materialization` helper dead, so that helper and the now-unused `publication_plans.hpp` include were removed in the same slice.

Preserved the externally used public lookup hooks:

- `make_named_prepared_result_register`
- `emitted_scalar_value_available`

## Suggested Next

Supervisor should review the Step 2 slice for commit readiness and decide whether the active runbook needs another lookup-surface pass or lifecycle review.

## Watchouts

The retained public hooks still have known external users in AArch64 lowering files, so they should remain public unless a later packet proves and implements a replacement owner. No test expectations or CMake translation-unit lists were changed.

## Proof

Ran the exact delegated proof command:

`(cmake --build --preset default && ctest --test-dir build -R '^backend_aarch64_' --output-on-failure) > test_after.log 2>&1`

Result: passed. The build completed and `ctest` reported 28/28 `backend_aarch64_` tests passed.

Proof log: `test_after.log`.
