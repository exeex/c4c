# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.2 packet in `src/backend/prealloc/prealloc.hpp`
by pushing the remaining short-circuit and compare-join entry helpers further
onto semantic ids: authoritative join-source lookup and materialized
compare-join context construction now resolve `BlockLabelId` once at the
boundary, param-zero compare-join entry now resolves both `BlockLabelId` and
`ValueNameId` before the typed helper graph continues, and
`resolve_prepared_compare_join_entry_target_labels(...)` now has a typed-id
entry path with the `bir::Block` overload reduced to a boundary wrapper.

## Suggested Next

Continue `plan.md` Step 3.2 in `src/backend/prealloc/prealloc.hpp` by auditing
the remaining source-branch and predecessor-label lookup entry helpers around
`find_authoritative_branch_owned_join_transfer(...)` so any residual
`std::string_view` overloads in the join-lookup family stay compatibility
wrappers only.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `liveness.cpp` still translates raw BIR spellings at the helper boundary
  where BIR enters the pass; follow-on packets should preserve that pattern
  instead of reintroducing raw-string maps inside the helper graph.
- The compare-join carrier path now interns carried result names when the
  helper first needs a `ValueNameId`; follow-on packets should keep that
  semantic-id capture at the helper boundary instead of dropping back to raw
  spelling comparisons deeper in the compare-join path.
- The new compare-join and short-circuit typed entry paths still translate BIR
  spellings once at the outer boundary; future packets should preserve that
  pattern rather than threading raw string labels deeper into helper internals.
- Keep the remaining string-view overloads in `prealloc.hpp` as compatibility
  wrappers only. Future packets should prefer typed-id entry paths and only
  translate BIR spellings once at the outer boundary.
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
build/test output in `test_after.log`. The build completed, and the backend
subset again reported the same four known failing tests already called out
above, matching the `test_before.log` failing-test set after the
`prealloc.hpp` short-circuit and compare-join entry helpers moved further onto
typed `BlockLabelId` and `ValueNameId` boundaries.
