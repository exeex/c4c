Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Return Retargeting Authority

# Current Packet

## Just Finished

Step 3 return retargeting authority repair completed in
`src/backend/mir/aarch64/codegen/memory.cpp`. Replaced
`find_memory_return_abi_register`'s raw before-return `move_bundles` scan with
`prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank`
through `context.function.move_bundle_lookups`, deriving the destination bank
from the load result type and preserving prepared ABI register conversion for
placement/name spelling.

## Suggested Next

Delegate the next prepared-authority packet for pointer-base global-symbol
recovery in memory record construction, keeping it separate from local-address
stack-object recovery and store-global publication state.

## Watchouts

This packet stayed confined to before-return retargeting. Local-address
stack-object recovery, pointer-base global-symbol recovery, and store-global
pending/duplicate publication still need separate prepared authority packets;
do not fold them into each other.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_return_lowering|backend_aarch64_call_boundary_owner|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary)$" 2>&1 | tee -a test_after.log'`

Build completed and all 6 selected tests passed.
