# Current Packet

Status: Active
Source Idea Path: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Replace value-home and block-entry publication reconstruction

## Just Finished

Step 2 replaced the block-entry publication value-home/current-publication
reconstruction route.

`current_block_entry_publication_register` now resolves the named BIR value
through prepared value identity: it uses `PreparedValueHomeLookups::value_ids`
when indexed lookups are present, falls back only to shared
`find_prepared_value_id` when no index is available, and then consumes
`find_indexed_prepared_value_home`. The old value-home iteration by rendered
prepared spelling was removed, so missing indexed value identity fails closed.

`value_has_current_block_entry_publication` now requires
`prepared_block_entry_publication_available`, and
`block_entry_move_clobbers_current_join_publication` now uses the same
prepared availability/status authority instead of hand-filtering only selected
unsupported statuses.

The AArch64 instruction-dispatch test now covers the successful indexed
block-entry publication register route, the missing-index fail-closed case
that would previously have matched by spelling, and unavailable publication
status cases for current-publication and clobber checks.

## Suggested Next

Implement Step 3 for branch-condition and fused-compare publications. Start by
replacing the `lower_missing_conditional_branch_condition_publication`
same-block producer scan with prepared branch/scalar-publication source
authority, then apply the same route to fused-compare operand publication if
the existing shared plan can express it.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this
publication repair with the open comparison, memory, ALU, calls, or other
AArch64 follow-up scopes. Reject value-home spelling scans, same-block producer
scans, raw move-bundle scans, lane-name matching, and expectation downgrades as
progress.

For later steps, branch-condition publication likely needs shared
producer/scalar-publication authority before removing the same-block producer
scan; before-return FPR retargeting needs a source-value/destination-bank
move-bundle query; local-slot address publication needs prepared address or
frame-offset authority instead of object/lane spelling; fixed-formal stores
should be classified by store-source publication planning.

Step 2 intentionally did not change `value_publication_may_read_register_index`;
the active plan treats that as register-hazard behavior unless it starts
reselecting semantic source or home facts.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 3/3 focused tests green. `test_after.log` contains the Step 2
proof.
