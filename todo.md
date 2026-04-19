# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Semantic Name Tables
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed `plan.md` Step 2 by generalizing `src/shared/text_id_table.hpp` into
reusable semantic name-table infrastructure, adding a prealloc-owned shared
name bank, and migrating the prepared liveness/regalloc name contracts from
public strings to typed semantic ids with updated test helpers.

## Suggested Next

Start `plan.md` Step 3 by migrating the next prepared/backend identity
surfaces that still publish symbolic strings, with priority on the first
addressing/control-flow contracts that upcoming CFG work will consume.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `src/backend/prealloc/prealloc.hpp` still has string-shaped identity surfaces
  outside the migrated liveness/regalloc family, especially in
  `PreparedAddressing` and `PreparedControlFlow`; keep the next packet focused
  on those contracts rather than widening into algorithm changes.
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
--output-on-failure -R '^backend_'` and captured the result in
`test_after.log`. The build completed and the backend subset reproduced the
same four known failing tests already present in `test_before.log`, so this
packet did not introduce a new subset regression.
