Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Steps 1-2 for
`ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md` by confirming the
first-pass extraction map and moving the clear addressing/layout helper family
from `src/backend/bir/lir_to_bir_memory.cpp` into the new
`src/backend/bir/lir_to_bir_memory_addressing.cpp` translation unit. The
backend notes test source list in `tests/backend/CMakeLists.txt` now includes
the new TU so the refactor builds on both the library and mirrored test path.

## Suggested Next

Execute `plan.md` Step 3 by extracting the clear pointer provenance and
addressed-object helper family into
`src/backend/bir/lir_to_bir_memory_provenance.cpp`, while leaving coordinator
glue and still-ambiguous helpers in `src/backend/bir/lir_to_bir_memory.cpp`.

## Watchouts

- Keep the first packet behavior-preserving; this runbook does not claim new
  backend capability.
- Do not force a `local` versus `global` split.
- `collect_local_scalar_array_slots` still lives in
  `src/backend/bir/lir_to_bir_memory.cpp`; treat it as coordinator-adjacent for
  now unless a later provenance pass proves a cleaner ownership home.
- `tests/backend/CMakeLists.txt` mirrors BIR source files for
  `backend_lir_to_bir_notes_test`, so future TU splits must keep that list in
  sync or the notes test will fail to link.
- Keep x86 renderer cleanup under idea `56`, not this runbook.

## Proof

Ran `bash -lc 'cmake --build --preset default 2>&1 | tee test_after.log &&
ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a
test_after.log'`. `test_before.log` was regenerated from a temporary clean
`HEAD` worktree using the same command because the previous root baseline was
for a narrower stale subset. `test_before.log` and `test_after.log` match on
the current four known failing backend-route tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`.
