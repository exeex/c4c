Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: moved the remaining tracked pointer-valued local-slot load
handoff around `tracks_pointer_value_slot` out of
`src/backend/bir/lir_to_bir_memory.cpp` and into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` by extending
`try_lower_tracked_local_pointer_slot_load` to own both the tracked
local-aggregate or array-element alias fast path and the existing reload-state
bookkeeping, so `lower_scalar_or_local_memory_inst` now only dispatches the
pointer-valued tracked-local-slot load to the local-slot owner.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining tracked local-slot load
logic still left in `src/backend/bir/lir_to_bir_memory.cpp`, especially the
`bir::TypeKind::I64` `global_address_ints` fast path and the plain
`LoadLocalInst` scalar fallback, and decide whether one more coherent
local-slot load wrapper belongs in
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` without absorbing broader
opcode admission checks or shared provenance helpers.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `try_lower_tracked_local_pointer_slot_load` now owns both the tracked
  local-pointer alias fast path and the existing reload-state bookkeeping,
  including `record_loaded_local_pointer_slot_state`, the paired
  `local_address_slots` / `global_pointer_slots` / `pointer_value_addresses`
  updates, and the fallback direct-slot `LoadLocalInst`; the next packet
  should preserve that ownership split instead of re-embedding tracked pointer
  load state or alias handoff in the coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- `diff -u test_before.log test_after.log` changed only timing lines; the
  functional result remained the same `4/5` subset with the pre-existing
  `c_testsuite_x86_backend_src_00205_c` backend-emitter limitation.
- Keep future packets focused on the next honest local-slot-owned load wrapper
  or helper family instead of pulling shared addressing helpers across
  ownership buckets or emptying `lir_to_bir_memory.cpp` for its own sake.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof again produced the same `4/5`
functional result recorded in `test_before.log`: the four backend
notes/handoff/dynamic-member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged with the same
backend-emitter limitation. `diff -u test_before.log test_after.log` changed
only timing lines, so `test_after.log` remains unchanged-behavior evidence for
this packet.
