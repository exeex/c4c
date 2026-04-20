# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Replace Value-Home Guessing With Prepared Lookups
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 3.2 (`Replace Value-Home Guessing With Prepared Lookups`) now covers the
next bounded scalar immediate-binary lane: `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
extends the minimal one-block i32 immediate-binary helper to consume a shared
`PreparedValueHomeKind::StackSlot` source home and preserve the required frame
allocation instead of assuming register-only parameter sourcing, while
`tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp` mutates the
prepared `add_one` contract to a stack-backed home and proves x86 reads that
authoritative prepared value-location data before executing the existing
`BeforeInstruction` and `BeforeReturn` move-bundle contract. Focused
`backend_x86_handoff_boundary` proof passed for the new lane, and the delegated
`^backend_` acceptance proof finished with the same four pre-existing failures
already present in `test_before.log`.

## Suggested Next

Continue Step 3.2 by extending the same prepared-home lookup route to the next
bounded scalar case that still assumes register-only sourcing, preferably a
small rematerializable-immediate lane or another one-instruction scalar shape
that can reuse the existing prepared move-bundle contract without widening into
Step 3.3 boundary-move execution.

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
- The bounded stack-home proofs still mutate the prepared value-location
  contract inside the scalar smoke tests; a later Step 3.2 slice should prefer
  a naturally produced stack-backed or rematerializable scalar fixture if one
  can stay bounded.
- `PreparedMovePhase::BlockEntry` is currently inferred from shared
  `phi_...` move reasons, while call/result/return bundles come from
  destination-kind classification in shared prepare; keep any later phase
  refinement shared instead of pushing it into x86.
- Expect the delegated `^backend_` proof subset to continue showing the same
  four pre-existing failures already present in `test_before.log` unless this
  packet unexpectedly touches unrelated backend routes:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran focused preflight `cmake --build --preset default && ctest --test-dir build
-j --output-on-failure -R '^backend_x86_handoff_boundary'`, which passed with
the new Step 3.2 stack-backed add-immediate prepared-home consumption check.
The delegated acceptance proof remains `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`,
with `test_after.log` as the canonical proof artifact for this packet. That
subset completed with the same four pre-existing failures already present in
`test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
