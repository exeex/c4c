# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Route non-edge scalar producer materialization through prepared source authority

## Just Finished

Completed Step 2 first repair route for non-edge scalar named-value
materialization in `emit_value_publication_to_register`.

The named-value producer selector now consumes prepared/shared authority through
`PreparedEdgePublicationSourceProducerLookups` and a guarded same-block
prepared producer context: the prepared producer must be in the current block,
before the requested materialization point, match the indexed instruction kind,
and match the requested result value. The same prepared route is also used by
the scratch-write hazard query so it no longer grows a same-block producer
lookup recursion independently.

The shared prepared source-producer fact now records `LoadGlobalInst` producers
as well as load-local, cast, binary, and select producers, preserving the
existing load-global materialization route without falling back to a local
same-block producer scan. Existing prepared value-home and block-entry
publication branches remain the first consumers when they can answer.

## Suggested Next

Continue Step 2 with the next prepared-authority gap: inspect whether select
chain materialization and direct-global select dependencies can consume a
prepared non-edge select-chain authority instead of relying on local dependency
walks inside the select materializer.

## Watchouts

The repaired route still reconstructs cast, binary, load-local, load-global,
and select emission after selecting a prepared source producer. Remaining Step 2
gap: select-chain and local-slot address subroutes still have their own local
authority questions and should be handled as separate narrow packets, not mixed
into this slice.

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
