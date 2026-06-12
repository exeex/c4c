Status: Active
Source Idea Path: ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Selected-Value-Chain Metadata Gap

# Current Packet

## Just Finished

Step 1: `Locate The Selected-Value-Chain Metadata Gap` reproduced and
localized the split-out idea 236 failure without implementation edits.

Exact failing assertion:

```text
scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership:
shared helper stopped publishing the true global-root selected-value chain
```

Minimal owner: prepared compare-join return-context metadata in
`src/backend/prealloc/control_flow.hpp`, centered on
`find_prepared_materialized_compare_join_return_context(...)` and its
`classify_computed_value(...)` recursion. The failure occurs before resolved
render-contract/x86 emission checks, after the helper has recognized the
pointer-backed same-module global base.

Expected selected-value-chain facts for the non-EdgeStoreSlot failing case:

- Published same-module global ownership order:
  `selected_zero_root`, `selected_zero_backing`,
  `selected_nonzero_root`, `selected_nonzero_backing`.
- True return context selected value:
  `PointerBackedGlobalI32Load` from `selected_zero_backing` at byte offset
  `0`, pointer root `selected_zero_root`, operations exactly
  `[Add immediate 4]`.
- False return context selected value:
  `PointerBackedGlobalI32Load` from `selected_nonzero_backing` at byte offset
  `0`, pointer root `selected_nonzero_root`, operations exactly
  `[Sub immediate 1]`.
- Both return contexts still carry the trailing return binary contract:
  `Xor immediate 3`.

## Suggested Next

Execute Step 2 in `src/backend/prealloc/control_flow.hpp`: repair
`classify_computed_value(...)` /
`find_prepared_materialized_compare_join_return_context(...)` so selected
values derived from pointer-backed same-module global roots retain exactly one
supported immediate operation in `PreparedComputedValue::operations` while
preserving the pointer-root/global-base facts.

Recommended Step 2 proof command:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

## Watchouts

- Preserve the idea 234 x86 compare-join stack-home repair as a regression
  guard.
- Preserve the idea 235 Route 6 consumed scalar i32 call-argument source repair
  as a regression guard.
- Keep the repair semantic: no fixture-name, label-text, assertion-string, or
  block-shape shortcuts.
- Fail closed when pointer-backed roots cannot be proven: missing/unresolved
  link names, non-`GlobalSymbol` memory address bases, pointer-address byte
  offsets other than zero, unresolved/extern/thread-local globals,
  non-`Ptr` pointer-root globals, cyclic computed-value names, non-i32 values,
  unsupported binary op shapes, or non-immediate chain operations.
- Nearby same-feature guards to preserve: direct same-module global selected
  values, non-pointer selected-value chains, fixed-offset global chains,
  pointer-backed direct global return contexts, EdgeStoreSlot variants,
  true/false passthrough variants, non-compare-entry-carrier rejection, and
  drifted/missing authoritative prepared branch contract rejection.
- Unchanged guard surfaces from this proof: `backend_prepared_lookup_helper`
  and `backend_x86_route_debug` pass, preserving Route 6 lookup behavior; the
  original idea 234 stack-home assertion does not reappear before this idea 236
  failure.

## Proof

Ran the delegated proof:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: failed with exit `8`, expected for Step 1 discovery. Passing:
`backend_prepared_lookup_helper`, `backend_x86_route_debug`. Failing:
`backend_x86_handoff_boundary` at the selected-value-chain assertion above.
Proof log: `test_after.log`.
