# Current Packet

Status: Active
Source Idea Path: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Route local-slot and aggregate-address publication through prepared address authority

## Just Finished

Step 5 replaced local-slot and aggregate-address publication's stack-object
spelling and `.0` lane recovery with prepared frame-address authority.

`PreparedAddressMaterializationLookups` now has the shared
`find_indexed_prepared_frame_address_offset_for_value` query. The query is keyed
by block label and prepared value name, requires indexed prepared
address-materialization authority, validates the prepared frame-slot id against
the stack layout, selects the latest reaching materialization, accepts duplicate
identical prepared facts, rejects conflicting same-instruction authority, and
fails closed when the indexed authority is absent.

`emit_local_slot_address_publication_to_register` now materializes the source
address from the shared prepared frame-address query instead of reconstructing a
stack object by rendered value spelling. `local_aggregate_address_frame_offset`
now consumes the same prepared value-keyed query instead of synthesizing a
`<aggregate>.0` lane name, and `local_slot_address_frame_offset` no longer
derives authority from stack-object spelling.

The prepared lookup helper test covers the shared frame-address query, including
latest-reaching selection, missing-index fail-closed behavior, future
materialization rejection, no lane-name recovery, duplicate identical authority,
and conflicting-authority rejection. The route test still covers the repaired
local aggregate address publication path through AArch64 lowering.

## Suggested Next

Implement Step 6 for fixed-formal store publication authority. Route the fixed
formal store path through prepared store-source/address authority without adding
local spelling scans or expectation downgrades.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this
publication repair with the open comparison, memory, ALU, calls, or other
AArch64 follow-up scopes. Reject value-home spelling scans, same-block producer
scans, raw move-bundle scans, lane-name matching, and expectation downgrades as
progress.

For later steps, fixed-formal stores should be classified by store-source
publication planning. Keep address publication using prepared value-keyed
address-materialization authority; do not reintroduce local object-name or lane
suffix matching.

Step 2 intentionally did not change `value_publication_may_read_register_index`;
the active plan treats that as register-hazard behavior unless it starts
reselecting semantic source or home facts.

Step 3 intentionally leaves target register spelling and source emission local
after prepared source-producer authority has authorized the fused-compare
producer-materialization route. Do not expand this packet into comparison,
ALU, memory, calls, or edge-copy emission refactors.

Step 5's frame-address query intentionally requires indexed prepared
address-materialization authority. A stack object name, rendered value spelling,
or aggregate lane suffix is not enough to authorize local address publication.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 3/3 focused tests green. `test_after.log` now contains the
Step 5 proof.
