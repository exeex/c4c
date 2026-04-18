Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: extracted the local aggregate resolver family from
`src/backend/bir/lir_to_bir_memory_addressing.cpp` into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp`, moving
`resolve_local_aggregate_gep_slot`,
`resolve_local_aggregate_pointer_array_slots`,
`resolve_local_aggregate_dynamic_pointer_array_access`,
`resolve_local_aggregate_gep_target`, and
`resolve_local_dynamic_aggregate_array_access` under the dedicated local-slots
owner while keeping `lower_scalar_or_local_memory_inst` in the main
coordinator TU; `can_reinterpret_byte_storage_view` now lives as a shared
class helper so the remaining addressing helpers can still use it.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining
`dynamic_local_aggregate_arrays` load/store handling in
`src/backend/bir/lir_to_bir_memory.cpp` and choose whether any residual
local-slot or local-aggregate mechanics still belong outside the coordinator,
without pulling opcode admission flow out of
`lower_scalar_or_local_memory_inst`.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `can_reinterpret_byte_storage_view` and
  `find_repeated_aggregate_extent_at_offset` are now the shared layout helpers
  left behind in addressing; avoid bouncing them into local-slots unless a
  later packet proves they are no longer shared.
- The next packet should be chosen by remaining coordinator-owned mechanics,
  not by line counts or by emptying `lir_to_bir_memory_addressing.cpp` for its
  own sake.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof reproduced the pre-existing
single failure `c_testsuite_x86_backend_src_00205_c` with no new subset
regression versus `test_before.log`; `test_after.log` is the canonical proof
log for this packet.
