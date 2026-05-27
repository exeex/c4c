Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Store-Source And Store-Global Publication State

# Current Packet

## Just Finished

Step 5 store-global publication state regression fix completed in
`src/backend/mir/aarch64/codegen/memory.cpp`,
`src/backend/prealloc/publication_plans.*`, and
`tests/backend/mir/backend_store_source_publication_plan_test.cpp`.
Kept pending store-global candidate selection and duplicate stack-home
publication state in prepared store-source publication plans, and fixed the
post-Step-5 duplicate `.Lselect_mat_*` label regression by making the prepared
pending query suppress replay for later stores inside an already-covered
contiguous publication run. The same prepared query now marks repeated source
values in one pending result as `duplicate_publication`, so AArch64 lowering
continues to consume prepared authority instead of a local duplicate set.

## Suggested Next

Delegate Step 6 broader validation or closure review if the supervisor accepts
the Step 5 regression fix. The store-global pending replay and duplicate-source
cases now have focused prepared-plan coverage.

## Watchouts

`dispatch.cpp` still carries the compatibility `published_store_global_stack_values`
argument because it is outside this packet's owned files, but the store-global
lowering path no longer consumes that set for duplicate or pending publication
authority. The shared prepared query emits pending publications only from the
first store-global in a contiguous select/load-global/binary/cast/store-global
run, preserving the intended before-first-store ordering without replaying
later stores.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|backend_store_source_publication_plan|backend_publication_plan_record|backend_prepared_lookup_helper|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00214_c)$" 2>&1 | tee -a test_after.log'`

Build completed and all 11 selected tests passed.
