# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Preserve block-entry and value-home helpers as prepared consumers

## Just Finished

Completed Step 2 by replacing the remaining direct-global select-chain root
metadata query in `prepare::find_prepared_direct_global_select_chain_dependency`
with prepared source-producer authority.

The prepared helper now consumes `PreparedEdgePublicationSourceProducerLookups`,
the prepared name table, the current block label, the BIR block, the named
source value, and the before-instruction index. It validates the indexed
producer fact against the BIR instruction, walks prepared load-global,
load-local, cast, binary, and select producer facts recursively, and returns
both direct-global reachability and root metadata without calling
`mir::find_same_block_named_producer_record` or
`mir::select_chain_contains_dependency`.

## Suggested Next

Begin Step 3 by preserving block-entry and value-home helper behavior while
keeping their producer and publication decisions on prepared authority. Start
with the narrowest helper that still falls back to local same-block producer
metadata for prepared block-entry/value-home decisions.

## Watchouts

Step 2 needed the store-source call path to pass the same prepared
source-producer authority into the shared helper, so `memory.cpp` has a narrow
signature-adaptation change for that direct caller. Keep local-slot address
repair out of the next route, and do not reintroduce same-block MIR scans into
prepared direct-global select-chain dependency planning.

## Proof

Ran the supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. The build completed and CTest reported 100% tests passed, 0
tests failed out of 163. Proof log: `test_after.log`.
