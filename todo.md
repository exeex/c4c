# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Replace Value-Home Guessing With Prepared Lookups
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 3.2 (`Replace Value-Home Guessing With Prepared Lookups`) now covers the
remaining minimal scalar passthrough and immediate-binary source-home lane:
`src/backend/prealloc/regalloc.cpp` publishes canonical parameter entry homes
into `PreparedValueLocations` from function ABI metadata instead of exposing
only regalloc-assigned destinations, and
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` now reads those shared
parameter homes plus the canonical `BeforeInstruction` and `BeforeReturn`
move bundles when rendering the bounded one-block i32 passthrough and
immediate-binary routes. The scalar smoke proof in
`tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp` was
tightened to assert the `id_i32` passthrough fixture publishes the expected
prepared parameter-home and return-bundle contract alongside the existing
`add_one` move-bundle checks, and the backend subset returned to the same
four pre-existing backend-route failures already present in `test_before.log`.

## Suggested Next

Continue Step 3.2 by extending the shared value-home lookup route beyond the
minimal register-backed passthrough/immediate-binary lane to the next bounded
scalar cases that still rely on emitter-local storage assumptions, especially
any stack-backed or rematerializable reads that can now consume the same
prepared contract without widening into Step 3.3 boundary-move work.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
- Parameter value homes now need to mean the canonical entry ABI location for
  consumers, not the later regalloc-assigned destination register that
  `BeforeInstruction` bundles may target.
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
--output-on-failure -R '^backend_' > test_after.log 2>&1`. The build
completed, `backend_x86_handoff_boundary` passed with the new Step 3.2
prepared-home contract checks and x86 lookup consumption, and the subset
finished with the same four pre-existing backend-route failures already
present in `test_before.log`.
