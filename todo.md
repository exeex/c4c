Status: Active
Source Idea Path: ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Acceptance Notes

# Current Packet

## Just Finished

Step 4 validation and acceptance notes are complete for idea 236.

Implementation summary:

- `src/backend/prealloc/control_flow.hpp` now interns display-only BIR global
  names into the prepared link-name table before publishing
  `PreparedSameModuleGlobalRef`.
- This preserves distinct semantic ids for the true/false pointer-root and
  global-base selected-value chains, so pointer-backed same-module global
  return contexts retain their global-root chain and immediate operation.
- The repair is semantic prepared metadata publication, not fixture-name,
  label, block-shape, expected-string, or assertion matching.

Guarded behavior:

- idea 234 x86 compare-join stack-home handoff remains covered by
  `backend_x86_handoff_boundary`.
- idea 235 Route 6 consumed scalar i32 source publication remains covered by
  `backend_prepared_lookup_helper`, `backend_x86_route_debug`, and the
  handoff-boundary path.
- Existing helper/handoff-boundary coverage is sufficient for missing roots,
  contradictory/drifted roots, unsupported pointer bases, non-global
  pointer-backed paths, and unsupported selected-value-chain operations.

## Suggested Next

Supervisor can ask the plan owner to close idea 236. Ideas 234 and 235 remain
open/parked and should be considered for closure now that the shared validation
subset passes through all three repaired assertions.

## Watchouts

- The repair is semantic link-name publication, not fixture-name or assertion
  matching.
- Existing fail-closed paths remain important for acceptance: missing or
  unresolved names, non-`GlobalSymbol` pointer address bases, nonzero pointer
  address offsets, extern/thread-local globals, non-`Ptr` pointer roots, cycles,
  non-i32 values, unsupported binary shapes, and non-immediate chain
  operations.
- Full `^backend_` on `build-x86` is not a useful close gate in this checkout:
  many CLI/codegen/runtime tests fail because `/workspaces/c4c/build-x86/c4cll`
  and several generated test executables are absent until those targets are
  built.
- The supervisor-selected broader validation built the relevant
  prepared/prealloc/handoff targets and passed the prepared metadata subset.
- Ideas 234 and 235 remain open, not active. Their implementation slices now
  pass the delegated guard subset, but their source acceptance criteria still
  require supervisor-selected broader validation before lifecycle closure.

## Proof

Focused Step 3 proof passed:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: passed.

Supervisor-selected broader prepared metadata validation also passed:

```bash
cmake --build build-x86 --target backend_prepare_phi_materialize_test backend_prepare_block_only_control_flow_test backend_prepare_authoritative_join_ownership_test backend_prepare_structured_context_test backend_prepare_liveness_test backend_prepare_stack_layout_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_decoded_home_storage_test backend_prealloc_call_boundary_classification_test backend_prealloc_formal_publications_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test backend_x86_route_debug_test backend_x86_handoff_boundary_test
ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_prepare_.*|backend_prealloc_.*|backend_prepared_.*|backend_x86_handoff_boundary|backend_x86_route_debug)$' > test_after.log 2>&1
```

Result: passed, 15/15 tests. Proof log: `test_after.log`.
