# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2.1
Current Step Title: Move Liveness Consumers To Prepared Semantic Id Boundaries
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.2.1 packet in
`src/backend/prealloc/liveness.cpp` by replacing the remaining liveness-local
raw lookup helpers with shared semantic-id resolution, caching each
function's `BlockLabelId` list once, and threading typed block/value ids
through successor collection, phi predecessor uses, and prepared-liveness
block construction so raw BIR spellings stay at one-time translation
boundaries instead of the internal helper graph.

## Suggested Next

Move to `plan.md` Step 3.2.2 in `src/backend/prealloc/prealloc.hpp` and clear
the next remaining compare/join continuation helpers that still translate
block or value spellings inside helper bodies rather than resolving
`BlockLabelId` and `ValueNameId` once at the boundary.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `liveness.cpp` now expects shared semantic-id resolvers plus one-time block
  label caching at the boundary; do not reintroduce direct
  `names.block_labels.find(...)` / `names.value_names.find(...)` lookups into
  its helper graph.
- Keep the remaining string-view overloads in `prealloc.hpp` as compatibility
  wrappers only. Follow-on packets should prefer typed-id entry paths and only
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
build/test output in `test_after.log`. The build completed and the backend
subset again reported the same four known failing tests already called out
above, matching `test_before.log`; the repo regression checker also passed
with `--allow-non-decreasing-passed` after this liveness semantic-id packet:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
