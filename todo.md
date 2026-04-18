Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: extracted the value-materialization helper family from
`src/backend/bir/lir_to_bir_memory.cpp` into the new semantic owner
`src/backend/bir/lir_to_bir_memory_value_materialization.cpp`, keeping the
coordinator TU focused on opcode lowering and wiring the new file into the
backend test build.

## Suggested Next

Continue `plan.md` Step 2 with the local-slot or local-aggregate mechanics
bucket: move `collect_local_scalar_array_slots`, `is_local_array_element_slot`,
and `parse_local_array_type` plus the closely tied slot-shape helpers into a
dedicated owner without pulling coordinator logic out of
`lower_scalar_or_local_memory_inst`.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- The next packet needs an ownership call on whether local aggregate slot-shape
  helpers belong in a dedicated local-slots TU or partially in addressing.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof reproduced the pre-existing
single failure `c_testsuite_x86_backend_src_00205_c` with no new subset
regression versus `test_before.log`; use `test_after.log` as the canonical
proof log for this packet.
