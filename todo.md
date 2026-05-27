Status: Active
Source Idea Path: ideas/open/53_aarch64_comparison_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Fused-Compare Operand Producer Authority

# Current Packet

## Just Finished

Step 3: Repair Fused-Compare Operand Producer Authority completed.
`lower_fused_compare_branch_from_emitted_cast` no longer uses
comparison-local cast/load producer scans or local constant evaluation for
fused-compare operand recovery. It now consumes the shared
`find_prepared_fused_compare_operand_producer` query, which is backed by
`PreparedEdgePublicationSourceProducerLookups` and returns producer kind,
instruction index, cast/load payloads, and optional same-block integer
constant facts. The existing prepared memory access path still owns the
frame-slot load address used for the narrow `ldrb` fallback.

## Suggested Next

Implement Step 4: replace materialized compare condition branch semantic
source discovery through `hooks.find_same_block_named_producer` with prepared
producer or scalar-publication authority keyed by the materialized condition
value.

## Watchouts

The shared fused-compare operand producer query requires named operands to be
published in prepared name tables. A focused AArch64 branch-control fixture was
updated to intern `%wide`; this is compatibility cleanup for the prepared
authority path, not an expectation rewrite. Materialized-condition,
block-entry, constant-RHS, and stack-home/select/load gates remain for later
packets.

## Proof

Passed. Ran the delegated command exactly:
`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_machine_printer|backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|c_testsuite_aarch64_backend_src_00164_c)$" 2>&1 | tee -a test_after.log'`
Build succeeded and CTest reported 8/8 tests passed. Proof log:
`test_after.log`.
