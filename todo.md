Status: Active
Source Idea Path: ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Nearby Metadata Stability

# Current Packet

## Just Finished

Step 2: `Repair Global-Root Chain Publication` completed the focused
prepared metadata repair in `src/backend/prealloc/control_flow.hpp`.

`resolve_prepared_bir_link_name_ref(...)` now interns display-only BIR global
names into the prepared link-name table before publishing
`PreparedSameModuleGlobalRef` facts. This preserves distinct true/false
same-module global identities for pointer-backed selected-value chains, so
`classify_computed_value(...)` can keep the correct pointer-root/global-base
facts and the selected-value operation chain:

- true arm: `PointerBackedGlobalI32Load` from `selected_zero_backing`, pointer
  root `selected_zero_root`, operations exactly `[Add immediate 4]`
- false arm: `PointerBackedGlobalI32Load` from `selected_nonzero_backing`,
  pointer root `selected_nonzero_root`, operations exactly `[Sub immediate 1]`

No expected strings, baselines, helper-oracle output, Route 6 consumed-plan
behavior, or x86 stack-home code were changed.

## Suggested Next

Execute Step 3: decide and run the supervisor-selected validation packet for
nearby selected-value-chain metadata stability.

Recommended packet:

- Inspect existing helper/handoff-boundary coverage around missing roots,
  contradictory roots, unsupported pointer bases, non-global pointer-backed
  paths, and unsupported selected-value-chain operations.
- Add or tighten focused fail-closed coverage only if existing coverage is
  insufficient for the shared `control_flow.hpp` repair.
- Re-run the delegated guard subset:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

If Step 3 confirms coverage is already sufficient, advance directly to Step 4
acceptance notes and supervisor-selected broader validation.

## Watchouts

- The repair is semantic link-name publication, not fixture-name or assertion
  matching.
- Existing fail-closed paths remain important for Step 3: missing or
  unresolved names, non-`GlobalSymbol` pointer address bases, nonzero pointer
  address offsets, extern/thread-local globals, non-`Ptr` pointer roots,
  cycles, non-i32 values, unsupported binary shapes, and non-immediate chain
  operations.
- Because `control_flow.hpp` is shared prepared metadata, supervisor should
  choose whether Step 3 needs focused coverage beyond the already-green
  handoff-boundary/helper/route-debug subset before acceptance.
- Ideas 234 and 235 remain open, not active. Their implementation slices now
  pass the delegated guard subset, but their source acceptance criteria still
  require supervisor-selected broader validation before lifecycle closure.

## Proof

Ran the delegated Step 2 proof:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: passed. `backend_prepared_lookup_helper`, `backend_x86_route_debug`,
and `backend_x86_handoff_boundary` all pass. Proof log: `test_after.log`.
