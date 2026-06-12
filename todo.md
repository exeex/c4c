Status: Active
Source Idea Path: ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Nearby Metadata Stability

# Current Packet

## Just Finished

Step 3: `Prove Nearby Metadata Stability` inspected the existing
helper/handoff-boundary coverage for the shared `control_flow.hpp`
selected-value-chain repair and found the focused coverage sufficient.

Existing coverage records the required stability surface:

- missing roots fail closed in `backend_prepared_lookup_helper` through the
  direct-global select-chain, scalar materialization, BIR identity, and Route 2
  record/index checks.
- contradictory/drifted authoritative roots fail closed in the x86
  handoff-boundary compare-join cases that reject drifted prepared branch
  contracts and missing authoritative prepared branch records instead of
  reopening raw recovery.
- unsupported pointer bases are guarded by `classify_computed_value(...)`
  rejecting pointer-backed loads unless the address base is a same-module
  `GlobalSymbol` with a resolved root name/id and zero address offset.
- non-global pointer-backed paths are guarded by the prepared helper
  non-global/mismatched-root fail-closed checks and by the x86 return-arm
  ownership checks for extern/thread-local/non-`Ptr` roots and missing or
  mismatched pointer initializers.
- unsupported selected-value-chain operations are guarded by the shared
  classifier accepting only supported immediate binary extensions and by the
  x86 renderer rejecting unsupported selected-value/trailing operations before
  emission.

No tests or implementation files were changed for Step 3.

## Suggested Next

Advance to Step 4 acceptance notes for idea 236, with supervisor-selected
broader validation as the next packet.

## Watchouts

- The repair is semantic link-name publication, not fixture-name or assertion
  matching.
- Existing fail-closed paths remain important for acceptance: missing or
  unresolved names, non-`GlobalSymbol` pointer address bases, nonzero pointer
  address offsets, extern/thread-local globals, non-`Ptr` pointer roots, cycles,
  non-i32 values, unsupported binary shapes, and non-immediate chain
  operations.
- The delegated Step 3 proof is focused. Since `control_flow.hpp` is shared
  prepared metadata, supervisor still owns whether Step 4 needs broader
  validation before lifecycle closure.
- Ideas 234 and 235 remain open, not active. Their implementation slices now
  pass the delegated guard subset, but their source acceptance criteria still
  require supervisor-selected broader validation before lifecycle closure.

## Proof

Ran the delegated Step 3 proof:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: passed. `backend_prepared_lookup_helper`, `backend_x86_route_debug`,
and `backend_x86_handoff_boundary` all pass. The delegated proof is sufficient
for the Step 3 coverage check; no narrow test gap was identified. Proof log:
`test_after.log`.
