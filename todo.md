# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.2 packet by moving the remaining
lookup-heavy liveness internals in `src/backend/prealloc/liveness.cpp` onto
semantic ids: dense-value state now stores `ValueNameId`, stack-object lookup
is keyed by typed value ids, block-index/successor/phi-predecessor lookup is
keyed by `BlockLabelId`, and raw BIR spellings are resolved only at the
liveness boundary before the helper graph carries typed ids internally.

## Suggested Next

Continue `plan.md` Step 3.2 in `src/backend/prealloc/prealloc.hpp` by pushing
the remaining join-transfer, branch-condition, and compare-join lookup helpers
onto typed `BlockLabelId` and `ValueNameId` entry paths, leaving the
`std::string_view` overloads as compatibility wrappers only.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `liveness.cpp` still translates raw BIR spellings at the helper boundary
  where BIR enters the pass; follow-on packets should preserve that pattern
  instead of reintroducing raw-string maps inside the helper graph.
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
above, matching the `test_before.log` failing-test set after the liveness
dense-value and block lookup helpers moved onto typed `ValueNameId` and
`BlockLabelId` internals.
