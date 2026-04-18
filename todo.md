Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: extracted the remaining coordinator-side local-pointer-slot GEP
fallback branch from `src/backend/bir/lir_to_bir_memory.cpp` into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` as
`try_lower_local_pointer_slot_base_gep`, so the local-slot owner now handles
dotted slot-name interpretation, `local_array_slots.find(base_name)`, and
`local_aggregate_slots.find(base_name)` while the coordinator only dispatches
that seam.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining local-slot and
value-materialization helpers in `src/backend/bir/lir_to_bir_memory.cpp` and
choose the next semantically coherent extraction bucket, keeping shared
addressing/provenance helpers outside that packet.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `try_lower_local_slot_pointer_gep`,
  `try_lower_local_array_slot_gep`, and
  `try_lower_local_pointer_array_base_gep` now sit alongside the new
  `try_lower_local_pointer_slot_base_gep`; the next packet should keep moving
  one honest ownership bucket at a time instead of re-centralizing that
  dispatch in the coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- Keep future packets focused on the next honest local-slot-owned wrapper
  instead of pulling shared addressing helpers across ownership buckets or
  emptying `lir_to_bir_memory.cpp` for its own sake.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof reproduced the same `4/5`
result recorded in `test_before.log`: the four backend notes/handoff/dynamic
member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged. `test_after.log`
remains the canonical proof log for this packet.
