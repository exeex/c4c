Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Local Address And Pointer-Base Authority

# Current Packet

## Just Finished

Step 4 local-address stack-object authority repair completed in
`src/backend/mir/aarch64/codegen/memory.cpp` and `src/backend/prealloc/*`.
Removed the AArch64 local-address store-value fallback that matched prepared
stack objects by value spelling or stack-object name equality. The rewrite now
consumes prepared frame-address materialization authority by prepared value id
and frame-slot id, with focused lookup coverage for the id-keyed query.

## Suggested Next

Delegate the next Step 5 packet for store-source/store-global publication state
if the supervisor wants to continue this route; pointer-base global-symbol
identity and local-address stack-object recovery have both been repaired in
separate slices.

## Watchouts

Store-global pending/duplicate publication remains out of scope for this packet.
The local-address rewrite requires a prepared stored value id plus a matching
prepared frame-address materialization; it no longer rederives authority from
`ValueNameId` spelling or stack-object names.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract)$" 2>&1 | tee -a test_after.log'`

Build completed and all 5 selected tests passed.
