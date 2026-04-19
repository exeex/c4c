# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The First Prepared Identity Surfaces
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed a `plan.md` Step 3 packet by migrating the prepared addressing
contract family from public strings to semantic ids, interning those ids in
`PreparedNameTables`, and updating the direct x86/test consumers to resolve
addressing names through `prepared.names`.

## Suggested Next

Continue `plan.md` Step 3 with the remaining prepared control-flow identity
surfaces so branch/join contracts publish semantic ids instead of strings and
upcoming CFG work can consume typed names end-to-end.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `PreparedAddressing` now depends on `PreparedNameTables` for block, value,
  pointer, and link-name decoding; future consumers must carry `prepared.names`
  through helper paths instead of reaching for raw strings.
- `src/backend/prealloc/prealloc.hpp` still has string-shaped identity surfaces
  in `PreparedControlFlow` and adjacent join/branch records; keep the next
  packet on those contracts rather than widening into algorithm changes.
- `PreparedNameTables` reattaches its semantic tables after copy/move; preserve
  that invariant if later packets add more semantic domains.
- The delegated backend subset still has the same four pre-existing failures
  from baseline:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'` and captured the build/test output in
`test_after.log`. The build completed and the backend subset reproduced the
same four known failing tests already present in `test_before.log`, so this
packet did not introduce a new subset regression.
