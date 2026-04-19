# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Migrate Remaining Prepared Lookup Helpers And Liveness Consumers
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 packet by moving the stack-layout prepared
addressing helpers onto semantic ids in `src/backend/prealloc/stack_layout.cpp`
and `src/backend/prealloc/prealloc.hpp`: stack-layout now resolves
`FunctionNameId` once per function and `BlockLabelId` once per block before
building prepared memory accesses, and the repeated named-value interning in
those helpers now flows through the shared `prepared_named_value_id(...)`
helper instead of open-coded raw spelling paths at each access site.

## Suggested Next

Continue `plan.md` Step 3.2 by migrating the remaining prepared/backend lookup
helpers that still accept raw value or block spellings, with priority on the
remaining `prealloc.hpp` and `liveness.cpp` lookup surfaces that still take
`std::string_view` names instead of typed ids.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- The new stack-layout helper boundary now assumes callers resolve
  `FunctionNameId` and `BlockLabelId` before entering the prepared-addressing
  builders. Follow-on packets should preserve that pattern instead of
  reintroducing function/block spelling intern calls deeper in the helper tree.
- Keep the remaining string-view lookup overloads in `prealloc.hpp` as
  compatibility wrappers only. Future packets should prefer typed-id entry
  paths and only translate BIR spellings once at the outer boundary.
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
above, matching `test_before.log` exactly after the stack-layout addressing
helpers moved onto typed function/block ids and the helper-local named-value
resolution was centralized through `prepared_named_value_id(...)`.
