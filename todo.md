Status: Active
Source Idea Path: ideas/open/53_aarch64_comparison_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Block-Entry And Constant RHS Fused-Compare Authority

# Current Packet

## Just Finished

Step 5: Repair Block-Entry And Constant RHS Fused-Compare Authority completed.
`lower_current_block_entry_fused_compare_branch` now consumes prepared
block-entry publication facts directly through
`collect_prepared_block_entry_publications` and
`prepared_block_entry_publication_available`, instead of the hook-shaped
publication register recovery. `lower_constant_rhs_fused_compare_branch` now
uses the prepared fused-compare operand producer query for RHS binary producer
and integer-constant facts, and checks block-entry publication through
prepared publication facts rather than the hook gate.

## Suggested Next

Implement Step 6: replace stack-home, select, load-local, and load-global
producer-kind gates with shared fused-compare operand producer-kind authority.

## Watchouts

The remaining local producer-kind gates are Step 6 work. Do not fold unrelated
publication cleanup or expectation rewrites into that packet.

## Proof

Passed. Ran the delegated command exactly:
`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer|backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_prepared_lookup_helper|c_testsuite_aarch64_backend_src_00164_c)$" 2>&1 | tee -a test_after.log'`
Build succeeded and CTest reported 8/8 tests passed. Proof log:
`test_after.log`.
