Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2 / Step 3: moved the coordinator's pointer-addressed load/store
dispatch out of `src/backend/bir/lir_to_bir_memory.cpp` and into the
provenance-owned helpers `try_lower_pointer_provenance_store` and
`try_lower_pointer_provenance_load` in
`src/backend/bir/lir_to_bir_memory_provenance.cpp`, so addressed-pointer and
reloaded local-slot pointer accesses now dispatch through the provenance owner
without changing lowering behavior.

## Suggested Next

Continue `plan.md` Step 4 by re-reading the remaining coordinator-side
pointer/global store-load dispatch after the new provenance extraction, and
decide whether the next honest packet is moving the surviving
global-provenance branch out of the coordinator or switching to a distinct
local-slot mechanics seam.

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
- Pointer-addressed and reloaded local-slot pointer dispatch now lives under
  `src/backend/bir/lir_to_bir_memory_provenance.cpp`; follow-on pointer/global
  branch work should keep pointer provenance routing there instead of moving it
  back into the coordinator.
- This packet's proof subset was tightened to the four directly relevant
  pointer-provenance route tests plus the backend notes and handoff surfaces,
  and all six passed under `test_after.log`.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `bash -lc 'set -o pipefail; cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
"^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_pointer_deref_observe_semantic_bir|backend_codegen_route_x86_64_pointer_param_direct_global_arg_observe_semantic_bir|backend_codegen_route_x86_64_pointer_param_global_pointer_value_arg_observe_semantic_bir|backend_codegen_route_x86_64_pointer_return_global_pointer_value_observe_semantic_bir)$"
| tee test_after.log'`.
The build passed, and the delegated narrow backend proof passed all six
selected tests:
`backend_lir_to_bir_notes`,
`backend_x86_handoff_boundary`,
`backend_codegen_route_x86_64_local_pointer_deref_observe_semantic_bir`,
`backend_codegen_route_x86_64_pointer_param_direct_global_arg_observe_semantic_bir`,
`backend_codegen_route_x86_64_pointer_param_global_pointer_value_arg_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_pointer_return_global_pointer_value_observe_semantic_bir`.
Proof log path: `test_after.log`.
