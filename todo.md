# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Route non-edge scalar producer materialization through prepared source authority

## Just Finished

Continued Step 2 by moving the non-edge direct-global select-chain boolean
dependency query onto prepared source-producer authority when those facts can
answer it.

`select_chain_contains_direct_global_load` now first traverses
`PreparedEdgePublicationSourceProducerLookups` for the current block and
materialization point, recursing through prepared cast, binary, and select
producer facts and recognizing prepared `LoadGlobal` facts directly. It falls
back to the legacy MIR dependency traversal only when prepared producer facts
are missing, ambiguous, out of block, out of range, or inconsistent with the
indexed instruction.

## Suggested Next

Complete Step 2 by adding a shared prepared select-chain dependency query that
returns both `contains_direct_global_load` and root metadata from
`PreparedEdgePublicationSourceProducerLookups`. The exact missing query shape
is: input prepared source-producer lookups, current block label, current BIR
block, named source value, and before-instruction index; output whether a
direct global load is reachable plus the root producer kind/select bit and root
instruction index. That would replace the remaining root-metadata use of
`mir::find_same_block_named_producer_record` in the direct-global select-chain
prepared helper.

## Watchouts

The non-edge boolean query no longer relies on local same-block traversal when
prepared producer facts are complete. The existing
`prepare::find_prepared_direct_global_select_chain_dependency` helper still
derives root metadata through MIR same-block producer records, so Step 2 should
remain open until that shared root-metadata query exists or the supervisor
chooses to split it. Keep local-slot address repair out of this route.

## Proof

Ran the supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. The build completed and CTest reported 100% tests passed, 0
tests failed out of 163. Proof log: `test_after.log`.
