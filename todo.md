Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2 / Step 3: moved the coordinator's remaining global/global-pointer
store bookkeeping out of `src/backend/bir/lir_to_bir_memory.cpp` and into the
new provenance-owned helper `try_lower_global_provenance_store` in
`src/backend/bir/lir_to_bir_memory_provenance.cpp`, so direct global stores,
global-object pointer stores, and addressed global-pointer stores now dispatch
through the provenance owner without changing lowering behavior.

## Suggested Next

Continue `plan.md` Step 4 by re-reading the remaining coordinator-side
pointer-array select and dynamic global pointer load dispatch around the load
fallback path, and decide whether the next honest extraction is another
provenance-owned pointer-value branch or a distinct value-materialization seam.

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
- Dynamic pointer/global value-selection helpers already live in
  `src/backend/bir/lir_to_bir_memory_value_materialization.cpp`; leave select
  synthesis there instead of recreating it inline in the coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- The latest accepted proof again changed only timing lines; the functional
  result remained the same `4/5` subset with the pre-existing
  `c_testsuite_x86_backend_src_00205_c` backend-emitter limitation, and the
  accepted baseline has not changed functionally.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$'
| tee test_after.log`.
The build passed, and the narrow backend proof again produced the same `4/5`
functional result as the prior baseline: the four backend
notes/handoff/dynamic-member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged with the same
backend-emitter limitation. `diff -u test_before.log test_after.log` changed
only timing lines. Because the delegated command uses `tee` without `pipefail`,
`ctest` still reported the known failing case inside `test_after.log` even
though the shell command itself exited `0`. Proof log path: `test_after.log`.
