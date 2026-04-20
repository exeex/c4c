# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Establish Minimal Move-Bundle Consumption For Scalar Home Queries
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Runbook review re-scoped the blocked Step 3 work so the current packet stays
on Step 3.1 (`Establish Minimal Move-Bundle Consumption For Scalar Home
Queries`): the failed value-home probe showed that minimal scalar x86
parameter/return routes cannot safely switch to direct
`PreparedBirModule::value_locations` lookup until x86 also executes the
shared `PreparedMoveBundle` plan before instruction 0 and before return. The
exploratory code was reverted after `backend_x86_handoff_boundary`
regressed, so the repo remains at the Step 2.2 implementation baseline with
the bounded Step 3.1 prerequisite now made explicit in `plan.md`.

## Suggested Next

Implement the repaired Step 3.1 directly: consume the matching
`BeforeInstruction` and `BeforeReturn` prepared move bundles for the minimal
single-block scalar family, recover destination registers from the shared
prepared value-home contract, and prove the bounded x86 handoff route no
longer depends on ABI/home guesses before moving on to the broader Step 3.2
value-home swap.

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
- A direct value-home swap on the minimal scalar param routes regressed
  `backend_x86_handoff_boundary`: for `add_one`, prepared homes currently land
  `%p.x` in `rbx` and `%sum` in `r11`, while the prepared move bundles carry a
  `BeforeInstruction` move for instruction 0 and a `BeforeReturn` move into
  `rax`. X86 must consume those shared moves together with the home lookup;
  using the value-home register by itself is wrong.
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
completed, and the attempted Step 3.1 patch introduced one additional failure
in `backend_x86_handoff_boundary` on top of the same four pre-existing
backend-route failures already present in `test_before.log`. The code
experiment was then reverted after the blocker was understood; `test_after.log`
is kept as the failed-step diagnosis artifact.
