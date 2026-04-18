Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: moved the remaining local-slot load dispatch wrapper out of
`src/backend/bir/lir_to_bir_memory.cpp` and into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` by adding
`try_lower_local_slot_load` plus the `LocalSlotLoadResult` tri-state, so the
local-slot owner now handles slot lookup, slot-type validation, and the
pointer-vs-nonpointer dispatch between
`try_lower_tracked_local_pointer_slot_load` and
`try_lower_nonpointer_local_slot_load` while the coordinator keeps only the
broader load-admission flow and dynamic-array fallback paths.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining coordinator-side
local-slot store/update handling in `src/backend/bir/lir_to_bir_memory.cpp`
and decide whether one thin local-slot-owned dispatch wrapper can move next
into `src/backend/bir/lir_to_bir_memory_local_slots.cpp` without absorbing
global-address admission, dynamic pointer-array selection, or broader opcode
routing.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `try_lower_local_slot_load` now owns local-slot load lookup, slot-type
  validation, and pointer-vs-nonpointer dispatch; future packets should keep
  that wrapper in the local-slot owner instead of re-embedding the split in
  the coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- `diff -u test_before.log test_after.log` changed only timing lines; the
  functional result remained the same `4/5` subset with the pre-existing
  `c_testsuite_x86_backend_src_00205_c` backend-emitter limitation.
- Keep future packets focused on the next honest local-slot-owned dispatch seam
  or helper family instead of pulling shared addressing helpers, dynamic
  pointer-array selection, or broad failure routing across ownership buckets.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof again produced the same `4/5`
functional result recorded in `test_before.log`: the four backend
notes/handoff/dynamic-member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged with the same
backend-emitter limitation. `diff -u test_before.log test_after.log` changed
only timing lines, so `test_after.log` at the repo root remains
unchanged-behavior evidence for this packet.
