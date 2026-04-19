# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.2 packet in `src/backend/prealloc/prealloc.hpp`
by migrating the param-zero compare-join compatibility wrappers around
`find_prepared_param_zero_branch_condition(...)` and
`find_prepared_param_zero_materialized_compare_join_context(...)` so the
`bir::Block`/`bir::Param` boundary overloads resolve `BlockLabelId` and
`ValueNameId` once through shared `resolve_prepared_block_label_id(...)` and
`resolve_prepared_value_name_id(...)` helpers before delegating only to the
typed-id helper paths.

## Suggested Next

Continue `plan.md` Step 3.2 in `src/backend/prealloc/prealloc.hpp` by auditing
the remaining compatibility wrappers that still open-code
`names.block_labels.find(...)` or `names.value_names.find(...)`, especially the
adjacent string-view and join-transfer lookup entry points, so the rest of this
prepared helper family also resolves semantic ids once at the boundary instead
of reintroducing raw spelling lookups deeper in the helper graph.

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
- The true/false join-source wrappers now follow that shared resolver pattern
  too; future packets should carry it through the remaining source-block
  wrapper family rather than mixing direct `find(...)` calls back in.
- The new compare-join and short-circuit typed entry paths still translate BIR
  spellings once at the outer boundary; future packets should preserve that
  pattern rather than threading raw string labels deeper into helper internals.
- The compare-join entry compatibility wrappers now route through
  `resolve_prepared_block_label_id(...)` and preserve the existing
  continuation-label fallback when source-label resolution fails; adjacent
  compatibility overloads should reuse that boundary helper instead of
  open-coding `names.block_labels.find(...)` in each wrapper.
- The param-zero branch-condition and materialized-compare-join wrappers now
  share both block-label and value-name boundary resolvers; follow-on packets
  should reuse those helpers instead of open-coding `find(...)` calls inside
  other compatibility overloads.
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
build/test output in `test_after.log`. The first build attempt failed inside
`src/backend/prealloc/prealloc.hpp` because the new boundary resolvers were
defined after their first use in the header; after adding forward declarations
for those owned helpers, the rerun build completed and the backend subset again
reported the same four known failing tests already called out above, matching
the current baseline after this packet’s param-zero wrapper migration:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
