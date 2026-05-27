Status: Active
Source Idea Path: ideas/open/53_aarch64_comparison_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Materialized Condition Branch Authority

# Current Packet

## Just Finished

Step 4: Repair Materialized Condition Branch Authority completed.
`lower_materialized_compare_condition_branch` no longer discovers the
condition producer through `hooks.find_same_block_named_producer`. It now uses
the shared `find_prepared_materialized_condition_producer` query, keyed by the
prepared condition value and backed by `PreparedEdgePublicationSourceProducer`
lookups, to recover the binary producer and instruction index before doing any
target-local operand/register publication work.

## Suggested Next

Implement Step 5: route current block-entry and constant RHS fused-compare
paths through prepared publication and branch-condition operand facts.

## Watchouts

The remaining `hooks.find_same_block_named_producer` uses in `comparison.cpp`
belong to later stack-home/select producer-kind gate repair, not the Step 4
materialized-condition path. Keep Step 5 bounded to block-entry and constant
RHS fused-compare authority.

## Proof

Passed. Ran the delegated command exactly:
`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_branch_control_lowering|backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_prepared_lookup_helper|c_testsuite_aarch64_backend_src_00164_c)$" 2>&1 | tee -a test_after.log'`
Build succeeded and CTest reported 7/7 tests passed. Proof log:
`test_after.log`.
