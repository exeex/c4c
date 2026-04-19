# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2.2
Current Step Title: Finish Residual Prepared Compare/Join Helper Cleanup
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed a `plan.md` Step 3.2.2 packet in
`src/backend/prealloc/prealloc.hpp` by removing the remaining unused
string-view compare/join compatibility wrappers for join-transfer,
branch-owned join-transfer, select-materialization, authoritative
branch-ownership, and incoming-transfer lookup helpers so the helper graph now
keeps `BlockLabelId` and `ValueNameId` as the only live entry contract in this
area.

## Suggested Next

Move to `plan.md` Step 3.3 and confirm the idea 62 starter surfaces around the
prepared control-flow/render entry points still expose typed semantic ids
cleanly, with any remaining raw spelling access limited to compatibility edges
outside the compare/join helper core.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- The compare/join helper family in `prealloc.hpp` no longer provides the old
  string-view join-transfer wrappers, so follow-on work should keep using the
  typed-id entry paths instead of reintroducing raw spelling overloads in this
  area.
- Step 3.3 should stay focused on confirming semantic-id starter surfaces, not
  expanding into broader CFG ownership or phi algorithm changes.
- The delegated backend subset still has the same four pre-existing failures
  from baseline:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log 2>&1` and captured the
build/test output in `test_after.log`. The build completed, the backend subset
again reported the same four known failures already present in
`test_before.log`, and no new regressions appeared for this Step 3.2.2
wrapper-removal packet:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
