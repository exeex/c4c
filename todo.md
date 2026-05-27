# Current Packet

Status: Active
Source Idea Path: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Replace before-return FPR ABI move-bundle scans

## Just Finished

Step 4 replaced before-return FPR ABI retargeting's raw move-bundle scan with
shared prepared move-bundle authority.

`PreparedMoveBundleLookups` now indexes before-return function-return ABI moves
by block, source value id, and destination register bank. The shared
`find_prepared_before_return_abi_move_by_source_and_destination_bank` query
requires prepared destination placement authority, rejects non-FPR destinations
for the FPR query, and fails closed when an explicit lookup set is missing the
source/bank key.

`memory_load_result_feeds_before_return_fpr_abi` no longer iterates
`value_locations->move_bundles` or parses destination register spelling. It now
consumes the shared source-value/FPR-return ABI lookup before allowing
frame-slot memory-result retargeting to the prepared FPR home.

The prepared lookup helper test covers indexed lookup, shared fallback
semantics, GPR rejection, missing-placement rejection, and missing-index
fail-closed behavior. The AArch64 HFA return-lane test still covers the
retargeted memory-result route and now also verifies that retargeting fails
closed when the prepared move lookup is empty.

## Suggested Next

Implement Step 5 for local-slot and aggregate-address publication. Replace
`local_slot_address_frame_offset` and `local_aggregate_address_frame_offset`
spelling/lane recovery with prepared address, pointer-base-plus-offset, or a
narrow shared frame-offset authority keyed by prepared value or stack object.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this
publication repair with the open comparison, memory, ALU, calls, or other
AArch64 follow-up scopes. Reject value-home spelling scans, same-block producer
scans, raw move-bundle scans, lane-name matching, and expectation downgrades as
progress.

For later steps, local-slot address publication needs prepared address or
frame-offset authority instead of object/lane spelling; fixed-formal stores
should be classified by store-source publication planning.

Step 2 intentionally did not change `value_publication_may_read_register_index`;
the active plan treats that as register-hazard behavior unless it starts
reselecting semantic source or home facts.

Step 3 intentionally leaves target register spelling and source emission local
after prepared source-producer authority has authorized the fused-compare
producer-materialization route. Do not expand this packet into comparison,
ALU, memory, calls, or edge-copy emission refactors.

Step 4's shared lookup intentionally requires prepared
`destination_register_placement` bank authority. A before-return move with only
target register spelling is not enough for the shared source-value/FPR ABI
query.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 3/3 focused tests green. `test_after.log` now contains the
Step 4 proof.
