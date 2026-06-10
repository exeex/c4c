Status: Active
Source Idea Path: ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select The Minimal Semantic Repair

# Current Packet

## Just Finished

Step 2 (`Select The Minimal Semantic Repair`): selected the narrow semantic
repair boundary for the selected f64 global readback dispatch debt. Repair
`src/backend/mir/aarch64/codegen/select_materialization.cpp` in
`materialize_direct_global_select_chain_call_argument(...)`: before calling
`prepare::find_prepared_value_home_for_bir_value(...)`, create a local
`std::optional<prepare::PreparedValueHomeLookups>` fallback when
`context.function.value_home_lookups` is null but
`context.function.value_locations` exists, assign it from
`prepare::make_prepared_value_home_lookups(context.function.value_locations)`,
and pass the resulting lookup pointer into
`find_prepared_value_home_for_bir_value(...)`.

This keeps ownership inside the helper that already decides whether a
direct-global select chain can be materialized as a call argument. The helper
currently has enough `value_locations` data to identify `%selected`'s prepared
FPR home, but a null `value_home_lookups` pointer makes the home lookup fail
early, so no `fcsel d20` is emitted before the existing `fmov d0, d20` ABI
move. Nearby AArch64 helpers already use the same local
`make_prepared_value_home_lookups(...)` fallback pattern when prepared lookup
attachments are absent.

This is preferred over changing the test because the Step 1 failure shows a
real missing materialization, not an invalid expectation. It is preferred over
broad call lowering or call ABI move routing because the ABI move already
exists; the missing behavior is source materialization into the prepared home.
It is preferred over Route 5 because join-source records should not carry call
ABI or layout policy for this local direct-global select materialization gap.

## Suggested Next

Step 3 should implement only the local fallback in
`materialize_direct_global_select_chain_call_argument(...)`, preserving the
existing direct-global select-chain and FPR materialization behavior. After the
code change, run:

```bash
(cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure) > test_after.log 2>&1
```

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
- Keep the fallback local to the direct-global select call-argument helper.
  Do not broaden source-selection policy, call ABI routing, or Route 5 state
  unless implementation proves this selected boundary is impossible.

## Proof

No new test run required for this todo-only Step 2 repair selection. Existing
Step 1 reproduction evidence remains in `test_after.log` from:

```bash
(cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure) > test_after.log 2>&1
```

Result: failed as expected for Step 1 with `expected selected f64 global
readback to feed call ABI move`; printed assembly includes the two
`g.f64.call` reloads and `fmov d0, d20`, but no preceding `fcsel d20`. Proof
log: `test_after.log`.
