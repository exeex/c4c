Status: Active
Source Idea Path: ideas/open/53_aarch64_comparison_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Conditional Branch Record Authority

# Current Packet

## Just Finished

Step 2: Repair Conditional Branch Record Authority completed.
`make_prepared_conditional_branch_record` now builds records from prepared
control-flow, prepared branch-condition, and prepared value-home facts only;
the raw BIR terminator is no longer passed into record construction.
`lower_prepared_conditional_branch_terminator` validates the retained raw
terminator separately for compatibility before consuming the prepared record,
including support for raw label spelling that maps back to prepared labels.
Focused branch-record tests were updated for the prepared-only record helper.

## Suggested Next

Implement Step 3: replace fused-compare cast/load/constant operand recovery in
`lower_fused_compare_branch_from_emitted_cast` with prepared operand,
source-producer, scalar-publication, memory-access, or a smallest shared
fused-compare operand producer query where existing lookups are not enough.

## Watchouts

Keep Step 3 bounded to fused-compare operand producer authority. Do not fold
materialized-condition producer lookup, block-entry publication cleanup, or
stack-home/select/load producer-kind gates into the same packet.

## Proof

Passed. Ran the delegated command exactly:
`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|backend_aarch64_scalar_record_contract|backend_prepared_lookup_helper|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call)$" 2>&1 | tee -a test_after.log'`
Build succeeded and CTest reported 9/9 tests passed. Proof log:
`test_after.log`.
