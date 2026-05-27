# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Route non-edge select-chain materialization through shared scalar authority

## Just Finished

Completed Step 6 root select-chain routing in
`dispatch_value_materialization.cpp`.

The non-edge select branch in `emit_value_publication_to_register` now requires
a prepared root value id before delegating to the shared
`emit_select_chain_value_to_register` emitter, and it seeds that shared emitter
with the prepared source-producer instruction index instead of the consumer's
local `before_instruction_index`. This keeps the root select-chain
materialization anchored to prepared scalar producer authority while preserving
the existing shared select-chain emission mechanics.

## Suggested Next

Execute Step 7 in the active runbook: audit
`emit_local_slot_address_publication_to_register` and route local-slot
address/frame-offset materialization through the prepared local-slot address
authority selected by the publication repair.

## Watchouts

A stricter recursive prepared-coverage gate was tested and rejected because an
existing supported stack-homed select publication route can contain an
intermediate select value with no prepared name/home. That is a real prepared
authority gap for possible follow-up, but this Step 6 packet kept the completed
slice to the prepared root select-chain authority inside the owned file.

Do not implement `globals.cpp` or `fp_value_materialization.cpp` global-load
consumer rewrites under this active plan; those remain follow-up work under the
separate global consumer idea unless lifecycle state changes.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Build succeeded; CTest passed 163/163 backend tests. Proof log:
`test_after.log`.
