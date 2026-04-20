# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Establish Minimal Move-Bundle Consumption For Scalar Home Queries
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 3.1 (`Establish Minimal Move-Bundle Consumption For Scalar Home
Queries`) now lands in the x86 prepared emitter for the minimal one-block
scalar immediate-binary family: `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
reads the shared prepared value-location function, narrows non-ABI home
registers like `r11` to their i32 spellings, and executes the canonical
`BeforeInstruction` and `BeforeReturn` move bundles before rendering the
final asm instead of assuming the result lives in the return ABI register.
The scalar smoke proof was tightened in
`tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp` to assert
that the `add_one` fixture publishes the expected prepared result-home and
move-bundle contract, and the backend handoff subset now returns to the same
four pre-existing backend-route failures without reintroducing the earlier
`backend_x86_handoff_boundary` regression.

## Suggested Next

Move into Step 3.2 (`Replace Value-Home Guessing With Prepared Lookups`) by
switching the remaining minimal scalar source/storage selection off ABI-based
register guesses and onto direct `PreparedBirModule::value_locations` home
queries, using the landed Step 3.1 move-bundle execution as the bounded
prerequisite for those broader lookup-based reads.

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
- Step 3.1 only covers the bounded one-block scalar immediate-binary lane.
  The direct parameter passthrough case still routes through the older minimal
  helper until Step 3.2 lifts source-home reads onto prepared value-location
  lookups more broadly.
- The delegated `^backend_` proof subset still has four pre-existing failing
  cases in both `test_before.log` and `test_after.log`:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log 2>&1`. The build
completed, `backend_x86_handoff_boundary` passed again with the new Step 3.1
route and tightened scalar smoke contract checks, and the subset finished with
the same four pre-existing backend-route failures already present in
`test_before.log`.
