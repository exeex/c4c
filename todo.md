# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers
Plan Review Counter: 10 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.2 packet in `src/backend/prealloc/prealloc.hpp`
by migrating the next prealloc compatibility-wrapper cluster around
`find_prepared_i32_immediate_branch_condition(...)` and the adjacent
`find_prepared_compare_branch_target_labels(...)` entry path so their
string-view and `bir::Block` boundary overloads resolve `BlockLabelId` and
`ValueNameId` through shared `resolve_prepared_block_label_id(...)` and
`resolve_prepared_value_name_id(...)` helpers before delegating to typed-id
lookup paths.

## Suggested Next

Continue `plan.md` Step 3.2 by migrating the next remaining prepared lookup
consumers that still translate BIR spellings inside helper bodies, with
priority on the out-of-scope `src/backend/prealloc/liveness.cpp` consumers and
then any residual compare-join continuation entry points in
`src/backend/prealloc/prealloc.hpp` that still open-code
`names.block_labels.find(...)` on `bir::Block` labels.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- Keep the remaining string-view overloads in `prealloc.hpp` as compatibility
  wrappers only. Follow-on packets should prefer typed-id entry paths and only
  translate BIR spellings once at the outer boundary.
- The immediate-branch lookup and compare-target-label wrapper cluster now
  resolves semantic ids at the boundary; do not reintroduce direct
  `names.block_labels.find(...)` or `names.value_names.find(...)` calls into
  those compatibility shims.
- `liveness.cpp` remains out of scope for this packet. When Step 3.2 moves
  there, preserve its one-time boundary translation pattern rather than
  threading raw spelling lookups back into helper internals.
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
build/test output in `test_after.log`. The build completed and the backend
subset again reported the same four known failing tests already called out
above, matching the current baseline after this packet’s immediate-branch and
compare-target-label wrapper migration:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
