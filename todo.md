Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Value-Home Id Recovery Scans

# Current Packet

## Just Finished

Step 2 value-home id recovery completed in
`src/backend/mir/aarch64/codegen/memory.cpp`. Removed the local
`find_value_home_id` and `find_unique_value_home_id` scans over
`value_locations.value_homes`; memory base validation, load/store identity, and
va-list field load/store records now recover ids through
`prepare::find_indexed_prepared_value_id` using prepared value-home lookup
authority. The pointer-base path still preserves the fail-closed ambiguous-home
contract by checking the prepared indexed home table.

## Suggested Next

Delegate the before-return retargeting packet: replace
`find_memory_return_abi_register`'s raw `move_bundles` scan with
`prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank`
through `context.function.move_bundle_lookups`, deriving the destination bank
from the load result type and preserving ABI register conversion behavior.

## Watchouts

The value-home id packet stayed confined to memory record/value-id recovery.
Local-address stack-object recovery, pointer-base global-symbol recovery, and
store-global pending/duplicate publication still need separate prepared
authority packets; do not fold them into return retargeting.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary)$" 2>&1 | tee -a test_after.log'`

Build completed and all 6 selected tests passed.
