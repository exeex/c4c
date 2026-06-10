Status: Active
Source Idea Path: ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize Selected F64 Readback Failure

# Current Packet

## Just Finished

Step 1 (`Reproduce And Localize Selected F64 Readback Failure`): reproduced the
monolithic dispatch failure with the delegated proof command. The failure is:
`expected selected f64 global readback to feed call ABI move`, with printed
assembly showing the two `g.f64.call` reloads, then `fmov d0, d20`, then
`bl consume_f64`, but no preceding `fcsel d20`.

Localized failing test/helper region:
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`:
`prepared_with_f64_select_global_call_argument()` feeds
`block_dispatch_publishes_f64_select_before_call_argument_move()`. The fixture
loads `%lhs/%rhs` from `g.f64.call`, selects `%selected` into prepared FPR home
`d20`, and has a prepared before-call move from `%selected` to ABI register
`d0`.

Suspected repair owners:
`src/backend/mir/aarch64/codegen/select_materialization.cpp`
(`materialize_direct_global_select_chain_call_argument`) and
`src/backend/mir/aarch64/codegen/calls.cpp`
(`lower_scalar_call_argument_producers`,
`materialize_call_boundary_source_to_destination`,
`fp_call_boundary_value_traces_to_global`). The selected F64 call argument
materialization path appears to fall through before emitting the FPR select
chain; the later call-boundary fallback preserves the stale prepared-home move
`fmov d0, d20`.

## Suggested Next

Step 2 should choose the minimal semantic repair that lets the direct-global
selected F32/F64 call-argument path materialize the selected value before the
ABI move. Start by checking whether `materialize_direct_global_select_chain_call_argument`
needs local/fallback value-home lookups when the block context lacks attached
`PreparedFunctionLookups`, or whether the call-boundary global-trace helper
must recognize select producers whose arms trace to global loads.

## Watchouts

- Keep this route limited to the ambient selected f64 global readback dispatch
  debt.
- Do not weaken, remove, downgrade, or mark unsupported the monolithic dispatch
  expectation.
- Do not use Route 5 join-source records to carry call ABI or layout policy.
- Do not special-case the exact test name, exact emitted assembly snippet, or a
  single named global instead of repairing source selection or call ABI move
  routing.
- The current failure is not a missing ABI move; the move is present as
  `fmov d0, d20`. The missing piece is materializing `%selected`/`d20` from the
  direct-global select chain before that move.
- Nearby i32 selected global call-argument tests attach prepared lookups in the
  test harness; this f64 fixture currently does not. Treat that as localization
  evidence, not approval to solve the bug by weakening or reshaping the test.

## Proof

Ran:

```bash
(cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure) > test_after.log 2>&1
```

Result: failed as expected for Step 1. Proof log: `test_after.log`.
