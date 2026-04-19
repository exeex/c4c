# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The First Prepared Identity Surfaces
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 packet by migrating the prepared
compare-join/computed-value helper surface from raw `param_name` and
`global_name` spellings to semantic ids (`ValueNameId` and `LinkNameId`),
then updating the compare-join resolver, x86 param-zero renderer/module
handoff, and joined-branch backend tests so those names resolve through
`PreparedNameTables` only at the final lookup/render boundary.

## Suggested Next

Continue `plan.md` Step 3 with the next prepared/public symbolic carrier that
still exposes raw names outside the helper boundary, likely the compare-join
same-module-global ownership vector or adjacent prepared stack-layout/regalloc
surfaces that still publish symbolic spellings instead of semantic ids.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `PreparedComputedBase` now carries semantic ids, not spellings. Future
  compare-join helpers must not compare `param.name`/`global.name` directly
  against those fields without resolving through `PreparedNameTables`.
- The compare-join helper chain now interns semantic ids while packaging
  render contracts. Callers that only hold a logically-const
  `PreparedBirModule` still need to preserve the backing `PreparedNameTables`
  instance rather than rebuilding ad hoc name tables.
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
`test_after.log`. The build completed, the short-circuit/compare-join backend
unit coverage passed with the computed-value semantic-id migration in place,
and the backend subset reproduced only the same four known failing tests
already present in `test_before.log`, so this packet did not introduce a new
subset regression.
