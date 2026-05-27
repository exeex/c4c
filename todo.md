# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Route non-edge scalar producer materialization through prepared source authority

## Just Finished

Completed Step 2 select-chain materialization repair for the non-edge
named-value route.

`find_same_block_select_producer` now consumes
`PreparedEdgePublicationSourceProducerLookups` before falling back to the
legacy MIR same-block scan. The prepared select producer must be in the current
block, precede the materialization point, be recorded as
`SelectMaterialization`, point at an indexed `SelectInst`, and match the
requested named result and type. This moves the select-chain materializer's
producer selection onto the same prepared source authority used by
`emit_value_publication_to_register` while preserving behavior when no complete
prepared select fact is available.

## Suggested Next

Continue Step 2 with the remaining narrow authority gap: direct-global
select-chain dependency detection still derives `contains_direct_global_load`
and root metadata through `mir::select_chain_contains_dependency` /
`mir::find_same_block_named_producer_record`. Decide whether existing prepared
source-producer facts are sufficient to answer that dependency query or whether
a dedicated prepared select-chain dependency lookup is needed.

## Watchouts

The repaired route still reconstructs cast, binary, load-local, load-global,
and select emission after selecting a prepared source producer. Remaining Step
2 gap: direct-global select-chain dependency discovery is still a local
dependency walk, and the local-slot address subroute remains separate work.
Do not mix local-slot address repair into the direct-global select-chain packet.

The new `LoadGlobal` prepared producer kind is now part of the shared source
producer fact and edge-copy producer identity checks can validate it. The
intentional repair in this packet is still the non-edge scalar publication
consumer; any broader edge-copy load-global behavior should be reviewed as a
separate packet.

## Proof

Ran the supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. The build completed and CTest reported 100% tests passed, 0
tests failed out of 163. Proof log: `test_after.log`.
