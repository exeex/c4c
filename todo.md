# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The First Prepared Identity Surfaces
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed a `plan.md` Step 3 packet by migrating the first prepared
control-flow identity surfaces from public strings to semantic ids, threading
those ids through `PreparedNameTables`, and updating the direct
prealloc/x86/test producer-consumer paths to resolve branch, join, block, and
slot identities through `prepared.names`.

## Suggested Next

Continue `plan.md` Step 3 with any remaining prepared control-flow lookup
surfaces that still expose raw strings outside this packet's owned files, then
only expand into the next prepared semantic-id family once the current
control-flow route is fully exhausted.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `PreparedControlFlowFunction`, `PreparedBranchCondition`,
  `PreparedJoinTransfer`, and `PreparedEdgeValueTransfer` now expose semantic
  ids publicly; future helper paths must carry `PreparedNameTables` instead of
  comparing branch/join/block/slot strings directly.
- The names-aware helper overloads in `src/backend/prealloc/prealloc.hpp` are
  now the stable external lookup surface for this packet. Prefer them over
  reopening string-based lookups in downstream consumers or tests.
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
`test_after.log`. The build completed, `backend_prepare_phi_materialize` and
`backend_x86_handoff_boundary` passed with the semantic-id migration in place,
and the backend subset reproduced only the same four known failing tests
already present in `test_before.log`, so this packet did not introduce a new
subset regression.
