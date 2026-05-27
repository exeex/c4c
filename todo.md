Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Local Address And Pointer-Base Authority

# Current Packet

## Just Finished

Step 4 pointer-base global-symbol authority repair completed in
`src/backend/mir/aarch64/codegen/memory.cpp` and `src/backend/prealloc/*`.
Added prepared pointer-base symbol identity to value-home/publication state and
made AArch64 pointer-base memory lowering consume that prepared `LinkNameId`
instead of recovering globals from value-name spelling or `@` prefixes. Fixed
the prepared-symbol publication branch to preserve `source_pointer_byte_delta`
for computed global subobject addresses.

## Suggested Next

Delegate the next Step 4 packet for local-address stack-object recovery, keeping
it separate from pointer-base global-symbol identity and store-global
publication state.

## Watchouts

This packet intentionally did not repair local-address stack-object recovery or
store-global pending/duplicate publication. Pointer-base global symbol lowering
now requires prepared symbol identity; fixtures that synthesize
`PointerBasePlusOffset` homes for globals should populate
`pointer_base_symbol_name`.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract)$" 2>&1 | tee -a test_after.log'`

Build completed and all 5 selected tests passed.
