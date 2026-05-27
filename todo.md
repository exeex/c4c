# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Route load-local source materialization through prepared memory facts

## Just Finished

Step 3 routed direct load-local edge source materialization through prepared
source-memory authority. `emit_edge_load_local_to_register_impl` now validates
direct prepared edge publications with
`PreparedEdgePublication::source_memory_*`,
`PreparedEdgePublicationSourceMemoryAccessStatus`, `PreparedMemoryAccess`, and
the source `PreparedValueHome`, emits frame-slot/pointer loads from the
prepared source-memory facts, and fails closed when direct publication memory
authority is missing instead of using value-home materialization or generic
publication recovery as a substitute for that missing authority.

The focused tests now cover direct load-publication emission from prepared
source memory, fail-closed behavior when the publication source-memory status is
missing, and prepared lookup exposure of the copied source-memory facts.

## Suggested Next

Implement Step 4 by routing block-entry edge-copy redundancy and parallel-copy
source matching through prepared move-bundle and edge-publication authority.

## Watchouts

`find_edge_named_producer` and `unique_branch_predecessor_context` remain
defined/exported, but the repaired source-producer materialization and hazard
paths no longer call them. Load-local materialization still allows the
non-direct/null-publication prepared-memory-access route; the direct prepared
publication route now requires available source-memory authority.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.
