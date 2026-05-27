# Current Packet

Status: Active
Source Idea Path: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Route branch-condition and fused-compare publications through shared source authority

## Just Finished

Step 3 routed branch-condition and fused-compare missing publication through
prepared source/scalar-publication authority.

`lower_missing_conditional_branch_condition_publication` no longer calls
`find_same_block_named_producer` to recover the condition producer. It now
requires the current `PreparedBranchCondition`, verifies that the prepared
condition value matches the BIR terminator condition, and materializes only
when `PreparedEdgePublicationSourceProducerLookups` supplies a same-block
producer fact. Missing source-producer authority fails closed.

`lower_missing_fused_compare_operand_publication` now checks the same prepared
source-producer authority before taking the producer-materialization route.
When no prepared source producer exists, it falls back only to a successful
`PreparedScalarPublicationPlan` over the prepared value home for register or
stack-slot publication. The selected-global fused-compare route remains
covered by the existing AArch64 instruction-dispatch test, and the new branch
condition test covers the repaired materialized-condition route plus the
missing-source-authority fail-closed case.

## Suggested Next

Implement Step 4 for before-return FPR ABI retargeting. Start by checking
whether `PreparedMoveBundleLookups` can answer the needed source-value plus
destination-bank query; if not, add the narrow shared lookup before replacing
the raw `value_locations->move_bundles` scan in
`memory_load_result_feeds_before_return_fpr_abi`.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this
publication repair with the open comparison, memory, ALU, calls, or other
AArch64 follow-up scopes. Reject value-home spelling scans, same-block producer
scans, raw move-bundle scans, lane-name matching, and expectation downgrades as
progress.

For later steps, before-return FPR retargeting needs a source-value plus
destination-bank move-bundle query; local-slot address publication needs
prepared address or frame-offset authority instead of object/lane spelling;
fixed-formal stores should be classified by store-source publication planning.

Step 2 intentionally did not change `value_publication_may_read_register_index`;
the active plan treats that as register-hazard behavior unless it starts
reselecting semantic source or home facts.

Step 3 intentionally leaves target register spelling and source emission local
after prepared source-producer authority has authorized the fused-compare
producer-materialization route. Do not expand this packet into comparison,
ALU, memory, calls, or edge-copy emission refactors.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 3/3 focused tests green. `test_after.log` now contains the
Step 3 proof.
