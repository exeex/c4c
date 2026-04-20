# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Build The Consumer View From Regalloc And Wire It Into The Prepared Module
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2.2 (`Build The Consumer View From Regalloc And Wire It Into The
Prepared Module`) now materializes `PreparedBirModule::value_locations` in
`src/backend/prealloc/regalloc.cpp`: each `PreparedRegallocFunction` is
translated into per-function `PreparedValueHome` records from assigned
register/stack-slot data, and shared `move_resolution` records are grouped
into `PreparedMoveBundle`s keyed by phase plus block/instruction coordinates
before the finalized consumer view is published on the prepared-module
handoff.

## Suggested Next

Move into Step 3.1 by switching the x86 prepared emitter to
`PreparedBirModule::value_locations` lookups for register/stack homes instead
of reading `stack_layout`, ABI register helpers, or regalloc-adjacent
conventions to rediscover storage locally.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
- `PreparedMovePhase::BlockEntry` is currently inferred from shared
  `phi_...` move reasons, while call/result/return bundles come from
  destination-kind classification in shared prepare; keep any later phase
  refinement shared instead of pushing it into x86.
- The delegated `^backend_` proof subset still has four pre-existing failing
  cases in both `test_before.log` and `test_after.log`:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'` with output captured in `test_after.log`.
The build completed, and the backend subset reproduced the same four failing
cases already present in `test_before.log`.
