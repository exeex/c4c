# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.2 packet in `src/backend/prealloc/prealloc.hpp`
by cleaning up the remaining source-branch and predecessor-label join lookup
wrappers: `find_branch_owned_join_transfer(...)`,
`find_select_materialization_join_transfer(...)`, and
`find_authoritative_branch_owned_join_transfer(...)` now resolve
`BlockLabelId` inputs once at the boundary through a shared helper and then
delegate to the typed-id entry paths instead of open-coding raw spelling
lookups in each overload.

## Suggested Next

Continue `plan.md` Step 3.2 in `src/backend/prealloc/prealloc.hpp` by auditing
the remaining true/false block-label entry helpers around
`find_authoritative_join_branch_sources(...)` and
`find_materialized_compare_join_context(...)` so the surviving
`std::string_view` overloads in that join-source family stay compatibility
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
- The join-lookup wrappers now share one `BlockLabelId` boundary resolver;
  follow-on packets should reuse that pattern for adjacent block-label wrapper
  families instead of reintroducing duplicated `names.block_labels.find(...)`
  logic.
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
`prealloc.hpp` source-branch and predecessor-label join-lookup wrappers moved
onto shared `BlockLabelId` boundary resolution and typed helper delegation.
