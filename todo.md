Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Unpublished Load-Local Producer Authority

# Current Packet

## Just Finished

Step 3: Repair Unpublished Load-Local Producer Authority completed.

Added an indexed prepared memory-access lookup by block label plus instruction
index and routed the unpublished load-local producer query through
`PreparedFunctionLookups::memory_accesses` together with
`PreparedFunctionLookups::edge_publication_source_producers`.

Removed the ALU-facing shared helper's fallback from result-value memory-access
scans to source-producer authority. The shared helper now requires indexed
source-producer facts for the value and consumer instruction boundary, then
uses indexed memory accesses for the load access and intervening store checks.

Preserved the no-intervening-store safety property in shared prepared authority:
an intervening store with missing/mismatched access or overlapping prepared
range still blocks the unpublished load-local source, while non-overlapping
stores remain allowed.

Adjusted the focused AArch64 prepared scalar ALU test context to attach
`PreparedFunctionLookups`, matching production traversal, and added
`backend_store_source_publication_plan` coverage for indexed unpublished
load-local source authority, missing source-producer rejection, non-overlapping
store allowance, and overlapping store rejection.

## Suggested Next

Proceed to the next supervisor-selected packet from the active plan. Keep it
bounded away from unpublished load-local source authority unless review finds a
regression in this slice.

## Watchouts

No direct users of the compatibility wrapper
`find_prepared_memory_access_by_result_value_name` remain under `src/`; only
the inline wrapper definition remains. The ALU unpublished load-local path now
requires `PreparedFunctionLookups`; hand-built test contexts that exercise this
path need to attach those lookups like traversal does.

## Proof

Ran the delegated proof command exactly:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_prepared_lookup_helper|backend_publication_plan_record|backend_store_source_publication_plan)$" 2>&1 | tee -a test_after.log'`

Result: build succeeded; all 8 focused tests passed. Proof log:
`test_after.log`.
