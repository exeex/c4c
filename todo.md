# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Producer And Consumer Surfaces
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 1 (`Inventory Producer And Consumer Surfaces`) now has a concrete shared
handoff scaffold in `src/backend/prealloc/prealloc.hpp`: `PreparedRegalloc`
was confirmed as the producer for per-function `values` and `move_resolution`,
the active x86 prepared route still leans on `stack_layout`, ABI register
helpers, and prepared control-flow/addressing lookups, and the header now
publishes additive `PreparedValueHome`, `PreparedMoveBundle`, and
`PreparedValueLocations` types plus direct lookup helpers on
`PreparedBirModule`.

## Suggested Next

Move into Step 2.2 by populating `PreparedBirModule::value_locations` from the
existing `PreparedRegalloc` and `PreparedStackLayout` outputs in
`src/backend/prealloc/regalloc.cpp`, keeping move-phase classification in
shared prepare instead of teaching x86 new home or ABI guesses.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
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
