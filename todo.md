Status: Active
Source Idea Path: ideas/open/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Acceptance Notes

# Current Packet

## Just Finished

Step 3 implementation completed for the selected Route 3 memory/source
stored-value helper-oracle production row.

Production agreement gate wired:

- `find_indirect_callee_stored_value_source(...)` now resolves the prepared
  same-block load-local stored-value source first.
- Route 3 identity is returned only when
  `find_route3_indirect_callee_stored_value_source_identity(...)` agrees with
  the prepared source on stored value plus store and load instruction indexes.
- Absent, invalid, ambiguous, mismatched, non-memory, or otherwise
  non-agreeing Route 3 evidence falls back to the prepared result when one
  exists; if prepared cannot prove a source, the existing caller fallback path
  remains authoritative.

Focused proof added:

- `block_dispatch_uses_route3_stored_indirect_callee_identity_for_selected_source`
  now uses the agreeing fixture
  `prepared_with_selected_stored_indirect_callee(false, false)` for the Route 3
  success path.
- New
  `block_dispatch_falls_back_for_stored_indirect_callee_route3_prepared_mismatch`
  uses `prepared_with_selected_stored_indirect_callee(false, true)`, where
  Route 3 can see the local-slot BIR fact but prepared memory authority points
  the store at a different slot. The test proves dispatch remains clean and
  does not emit the Route 3 `csel x9` retarget before `blr x9`.
- Existing absent-Route-3 fallback and nearby memory/source record tests remain
  in the delegated proof subset.

## Suggested Next

Step 4 - Validate And Prepare Acceptance Notes. Supervisor review can check the
production agreement gate and decide whether the active runbook is ready for
acceptance or needs broader validation.

## Watchouts

- Do not change helper-oracle strings, wrappers, expected strings, prepared
  addressing printer output, address formation, materialization, addressing
  legality, final operands, target policy, baselines, or supported-path
  contracts.
- The production gate intentionally compares the target-neutral identity fields
  used by this consumer, not target-addressing fields; prepared remains the
  authority for target policy and final operands.

## Proof

Delegated proof passed after the Step 3 implementation:
`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_memory_operand_records_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_records)$' --output-on-failure > test_after.log 2>&1`.

`test_after.log` contains 4/4 passing tests:
`backend_prepared_lookup_helper`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_prepared_memory_operand_records`, and
`backend_aarch64_memory_operand_records`.

Supervisor-run broader validation passed:
`cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.

Result: 29/29 passing tests.
