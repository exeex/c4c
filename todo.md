Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2 / Step 3: moved the coordinator's remaining dynamic pointer-array
load fallback out of `src/backend/bir/lir_to_bir_memory.cpp` and into the
value-materialization-owned helper
`try_lower_dynamic_pointer_array_load` in
`src/backend/bir/lir_to_bir_memory_value_materialization.cpp`, so local and
global pointer-array select materialization now dispatch through that owner
without changing lowering behavior.

## Suggested Next

Continue `plan.md` Step 4 by re-reading the remaining coordinator-side
load-path pointer handling after the new value-materialization extraction, and
decide whether the next honest packet is a provenance-owned addressed/global
pointer branch or a distinct local-slot mechanics seam.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `try_lower_global_provenance_load`, `try_lower_global_provenance_store`, and
  the addressed-pointer scalar helpers now live under
  `src/backend/bir/lir_to_bir_memory_provenance.cpp`; follow-on packets should
  keep global/pointer alias recovery and addressed access rules in the
  provenance owner instead of moving them back into the coordinator.
- `try_lower_local_slot_load` and `try_lower_local_slot_store` still own the
  local-slot-specific load/store dispatch and bookkeeping wrappers; do not use
  provenance extraction as a reason to pull local-slot alias or address updates
  back out of that owner.
- Dynamic pointer/global value-selection helpers now include
  `try_lower_dynamic_pointer_array_load` under
  `src/backend/bir/lir_to_bir_memory_value_materialization.cpp`; keep follow-on
  select materialization in that owner instead of recreating it inline in the
  coordinator.
- This packet's proof subset was tightened to the four directly relevant
  backend tests, and all four passed under `test_after.log`.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `bash -lc 'set -o pipefail; cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
"^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir)$"
| tee test_after.log'`.
The build passed, and the delegated narrow backend proof passed all four
selected tests:
`backend_lir_to_bir_notes`,
`backend_x86_handoff_boundary`,
`backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`.
Proof log path: `test_after.log`.
