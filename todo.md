Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: extracted the `local_pointer_slots` load-path local reload
bookkeeping from `src/backend/bir/lir_to_bir_memory.cpp` into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` as
`record_loaded_local_pointer_slot_state`, so the local-slot owner now rebuilds
`local_slot_pointer_values`, array-backed `local_aggregate_slots`, and
`local_pointer_array_bases` for reloaded local-slot pointers while
`lower_scalar_or_local_memory_inst` keeps only the surrounding admission and
provenance dispatch.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining `local_pointer_slots`
load-path pointer provenance updates in
`src/backend/bir/lir_to_bir_memory.cpp`, and choose whether the
`local_address_slots` / `global_pointer_slots` / `pointer_value_addresses`
bookkeeping is the next coherent `lir_to_bir_memory_local_slots.cpp` seam
without pulling shared global-address reasoning or non-local ownership out of
the coordinator.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `record_loaded_local_pointer_slot_state` now owns the local-slot reload
  bookkeeping for `local_slot_pointer_values`, `local_aggregate_slots`, and
  `local_pointer_array_bases`; the next packet should preserve that ownership
  split instead of re-embedding local reload state construction in the
  coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- Keep future packets focused on the next honest local-slot-owned wrapper or
  helper family instead of pulling shared addressing helpers across ownership
  buckets or emptying `lir_to_bir_memory.cpp` for its own sake.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof reproduced the same `4/5`
result recorded in `test_before.log`: the four backend notes/handoff/dynamic
member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged with the same
backend-emitter limitation. `test_after.log` remains the canonical proof log
for this packet.
