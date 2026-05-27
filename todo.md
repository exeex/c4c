Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Local Address And Pointer-Base Authority

# Current Packet

## Just Finished

Step 4 local-address lookup regression fix completed in
`src/backend/mir/aarch64/codegen/memory.cpp` and `src/backend/prealloc/*`.
Tightened the prepared frame-address lookup so it rejects frame slots backed
only by non-addressable regalloc spill objects, preserving the semantic/id-keyed
route without value-name or stack-object name matching. Also repaired the
store-local publication path so simple pointer casts with stack homes publish
their prepared stack slot before a later store-local consumes that slot; this
fixes the `extend_brk` null return in `c_testsuite_aarch64_backend_src_00214_c`.

## Suggested Next

Delegate the next Step 5 packet for store-source/store-global publication state
if the supervisor wants to continue this route; the Step 4 local-address
regression is repaired and the focused route remains id-keyed.

## Watchouts

Store-global pending/duplicate publication remains out of scope for this packet.
The frame-address lookup now requires a prepared stored value id plus a matching
prepared frame-address materialization whose frame slot belongs to an
addressable/permanent stack object; ordinary regalloc spill slots are not local
address authority. The cast stack-home publication repair is limited to
store-local source publication and does not change store-global state.

## Proof

Ran the supervisor-selected proof command and wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(c_testsuite_aarch64_backend_src_00214_c|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_prepared_lookup_helper)$" 2>&1 | tee -a test_after.log'`

Build completed and all 7 selected tests passed.
