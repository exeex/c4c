Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: moved the remaining local-slot store dispatch wrapper out of
`src/backend/bir/lir_to_bir_memory.cpp` and into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` by adding
`try_lower_local_slot_store` plus the `LocalSlotStoreResult` tri-state, so the
local-slot owner now validates destination slots, maintains the local pointer
alias and address bookkeeping for `i64` and `ptr` local-slot stores, and emits
the final `bir::StoreLocalInst` while the coordinator keeps only the broader
global-address, dynamic-array, and fail-routing admission flow.

## Suggested Next

Continue `plan.md` Step 4 by re-reading the remaining coordinator-side helper
mix in `src/backend/bir/lir_to_bir_memory.cpp` and decide whether the next
honest extraction has shifted to a value-materialization or other non-local-slot
ownership seam rather than forcing another thin local-slot packet.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `try_lower_local_slot_load` and `try_lower_local_slot_store` now own the
  local-slot-specific load/store dispatch and bookkeeping wrappers; future
  packets should keep those wrappers in the local-slot owner instead of
  re-embedding local-slot alias or address updates in the coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- The latest accepted proof again changed only timing lines; the functional
  result remained the same `4/5` subset with the pre-existing
  `c_testsuite_x86_backend_src_00205_c` backend-emitter limitation, and the
  accepted baseline has been rolled forward into `test_before.log`.
- Do not stretch the local-slot bucket to absorb shared addressing,
  global-address admission, dynamic pointer-array selection, or broad failure
  routing just to keep this route going; if the remaining seam is no longer
  honestly local-slot-owned, switch the next packet to the stronger owner.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof again produced the same `4/5`
functional result as the prior baseline: the four backend
notes/handoff/dynamic-member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged with the same
backend-emitter limitation. `diff -u test_before.log test_after.log` changed
only timing lines, so the accepted proof was rolled forward into the canonical
baseline at `test_before.log`.
