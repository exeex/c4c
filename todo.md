# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The First Prepared Identity Surfaces
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 packet by migrating the prepared
stack-layout object/frame-slot function carrier from raw function-name
spellings to `FunctionNameId`, threading `PreparedNameTables` through
stack-layout object collection/regalloc hinting, updating liveness and x86
prepared local-slot consumers to compare semantic ids directly, and extending
the stack-layout backend test coverage to assert those contracts through
`PreparedNameTables`.

## Suggested Next

Continue `plan.md` Step 3 with the next prepared/public symbolic carrier that
still exposes raw spellings, likely the stack-layout object `source_name`
surface and its direct prealloc/coalescing users so stack objects can stop
publishing untyped source-name strings once a bounded semantic-domain split is
chosen.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `PreparedStackObject::function_name` and `PreparedFrameSlot::function_name`
  now depend on the shared `PreparedNameTables` instance that interned the
  owning function name. Future direct consumers must not compare those fields
  against raw `bir::Function::name` spellings.
- `build_prepared_module_local_slot_layout` now needs `PreparedNameTables`
  when it wants prepared frame-slot offsets from semantic stack-layout data.
  If a later packet widens that x86 helper surface, keep the stack-layout-only
  fallback behavior coherent.
- This packet intentionally left `PreparedStackObject::source_name` as a raw
  string because the remaining fan-out runs through alloca/copy coalescing and
  other stack-layout helpers. The next slice should choose a domain split
  before converting that carrier.
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
kept `backend_prepare_stack_layout`, `backend_prepare_liveness`, and the x86
handoff boundary coverage green with semantic stack-layout function ids in
place, and the run reproduced only the same four known failing tests already
present in `test_before.log`, so this packet did not introduce a new subset
regression.
