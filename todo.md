Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar Load Source Authority

# Current Packet

## Just Finished

Step 2: Repair Scalar Load Source Authority completed.

Added `PreparedMemoryAccessLookups` to `PreparedFunctionLookups`, indexed by
result value name and by result value id. The unique-result query preserves the
previous duplicate-result ambiguity behavior by returning null unless exactly
one prepared memory access matches.

Routed AArch64 `make_prepared_scalar_load_source` through
`find_unique_indexed_prepared_memory_access_by_result_value_id` on
`context.function.prepared_lookups->memory_accesses`, so the ALU scalar load
source no longer calls the scan-shaped
`find_prepared_memory_access_by_result_value_name` helper directly.

Added focused `backend_prepared_lookup_helper` coverage for unique lookup by
name, unique lookup by value id, duplicate ambiguity, duplicate retention, and
invalid value names.

## Suggested Next

Proceed to the next supervisor-selected Step 3 packet from the active plan,
keeping it bounded away from scalar load-source lookup authority unless review
finds a regression in this slice.

## Watchouts

The inline compatibility wrapper
`find_prepared_memory_access_by_result_value_name` still has publication-plan
users in `src/backend/prealloc/publication_plans.cpp`; those were outside this
packet and remain as compatibility-wrapper users for a later shared-query
cleanup. No ALU scalar load-source caller remains on that helper.

## Proof

Ran the delegated proof command exactly:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_prepared_lookup_helper)$" 2>&1 | tee -a test_after.log'`

Result: build succeeded; all 6 focused tests passed. Proof log:
`test_after.log`.
