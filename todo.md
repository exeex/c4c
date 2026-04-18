Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 4 for
`ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md` by confirming the
remaining scalar subobject admission checks in
`src/backend/bir/lir_to_bir_memory.cpp` into one file-local helper so
`lower_scalar_or_local_memory_inst` reads more directly as the opcode-lowering
coordinator. The store/load branches now reuse the shared helper without
changing lowering behavior or widening the refactor beyond the coordinator TU.

## Suggested Next

Execute `plan.md` Step 5 by treating the current memory split as refactor-only
and deciding whether acceptance needs any broader validation beyond the active
`^backend_` proof surface before the runbook can be closed or advanced.

## Watchouts

- Keep the first packet behavior-preserving; this runbook does not claim new
  backend capability.
- Do not force a `local` versus `global` split.
- `collect_local_scalar_array_slots` still lives in
  `src/backend/bir/lir_to_bir_memory.cpp`; treat it as coordinator-adjacent for
  now unless a later packet proves a cleaner ownership home without changing
  semantics.
- The current `^backend_` proof still shows the same four known backend-route
  failures, so Step 5 should treat those as inherited baseline rather than new
  regressions from this coordinator cleanup.
- Keep x86 renderer cleanup under idea `56`, not this runbook.

## Proof

Ran `bash -lc 'cmake --build --preset default 2>&1 | tee test_after.log &&
ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a
test_after.log'`. `test_after.log` still shows the same four known failing
backend-route tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`.
