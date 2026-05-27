Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Store-Source And Store-Global Publication State

# Current Packet

## Just Finished

Step 5 store-global publication state repair completed in
`src/backend/mir/aarch64/codegen/memory.cpp`,
`src/backend/prealloc/publication_plans.*`, and
`tests/backend/mir/backend_store_source_publication_plan_test.cpp`.
Moved pending store-global candidate selection and duplicate stack-home
publication state into prepared store-source publication plans, then rewired
AArch64 store-global publication emission to consume those plan fields instead
of the local forward scan and duplicate set as semantic authority. The prepared
store-global access lookup is block-label keyed so same-index stores in other
blocks cannot supply publication authority.

## Suggested Next

Delegate the next Step 5 or Step 6 packet for any remaining store-source
publication audit or broader validation before closure. The store-global
pending/duplicate state path now has focused prepared-plan coverage.

## Watchouts

`dispatch.cpp` still carries the compatibility `published_store_global_stack_values`
argument because it is outside this packet's owned files, but the store-global
lowering path no longer consumes that set for duplicate or pending publication
authority. The shared prepared query preserves the previous pending publication
ordering for contiguous select/load-global/binary/cast/store-global runs.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_store_source_publication_plan|backend_publication_plan_record|backend_prepared_lookup_helper|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00214_c)$" 2>&1 | tee -a test_after.log'`

Build completed and all 9 selected tests passed.
