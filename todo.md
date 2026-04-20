# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Replace Value-Home Guessing With Prepared Lookups
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 3.2 (`Replace Value-Home Guessing With Prepared Lookups`) now covers the
next bounded scalar stack-home lane: `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
extends the minimal one-block i32 passthrough route to consume a shared
`PreparedValueHomeKind::StackSlot` source home and wrap the return with the
required frame allocation instead of assuming register-only sourcing, while
`tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp` mutates the
prepared `id_i32` contract to a stack-backed home and proves x86 follows that
authoritative prepared value-location data rather than the raw parameter ABI
carrier. The delegated `^backend_` proof finished with the same four
pre-existing backend-route failures already present in `test_before.log`.

## Suggested Next

Continue Step 3.2 by extending the same prepared-home lookup route to the next
bounded scalar case that still assumes register-only sourcing, preferably a
stack-backed immediate-binary or similarly small rematerializable read that can
reuse the existing `BeforeInstruction` and `BeforeReturn` bundle contract
without widening into Step 3.3 boundary-move execution.

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
- The new stack-home passthrough proof currently mutates the prepared
  value-location contract inside the scalar smoke test; the next slice should
  prefer a naturally produced stack-backed or rematerializable scalar fixture
  if one can stay bounded inside Step 3.2.
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
stack-backed prepared-home consumption check, and the subset finished with the
same four pre-existing backend-route failures already present in
`test_before.log`.
