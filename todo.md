Status: Active
Source Idea Path: ideas/open/233_phase_e3_route7_materialized_condition_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Fallback And Nearby Same-Feature Stability

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the selected materialized-condition
helper-oracle row in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Assertions added inside
`verify_prepared_bir_comparison_condition_producer_equivalence()`:

- Added a local `route7_materialized_condition_agrees(...)` predicate that
  accepts Route 7 evidence only when the indexed materialized-condition record,
  `route_index_validate_materialized_condition_reference(...)`, prepared
  producer, BIR identity, and lhs/rhs operand reference validation agree.
- Tightened the positive `%cond` row so Route 7/BIR/prepared agreement is
  required for the same binary producer, instruction index `3`, condition value,
  prepared condition value name, and lhs/rhs producer facts.
- Kept the existing absent-route fallback and added local fail-closed checks for
  invalid/stale Route 7 reference, duplicate `%cond` materialized-condition
  record, Route 7/prepared producer mismatch, unfused/non-comparison lookup, and
  missing lhs producer evidence.
- Preserved the existing helper-oracle failure strings; no expected strings,
  baselines, branch-control output, machine-printer output, wrappers, final
  assembler behavior, route-index public contracts, or supported/unsupported
  contracts were changed.

## Suggested Next

Delegate Step 3 to prove fallback and nearby same-feature stability.

Suggested packet:

- Review the Step 2 helper-oracle diff against
  `backend_aarch64_branch_control_lowering_test.cpp` same-feature cases and add
  only focused tests if the supervisor wants extra coverage beyond the current
  local helper-oracle assertions plus delegated branch-control and instruction
  dispatch proof.

## Watchouts

- Step 2 stayed helper-oracle-local and did not expose production agreement
  predicates.
- Remaining Step 3 proof gaps are mostly acceptance confidence rather than known
  missing local assertions: compare the helper-oracle row with nearby
  branch-control positive/fallback cases, and decide whether same-feature
  stability needs additional focused tests or only documented proof from the
  already-run subset.
- Keep rejecting any follow-up that changes helper-oracle strings, expected
  strings, baselines, branch-control output, machine-printer output, wrappers,
  final assembler behavior, route-index public contracts, or supported/unsupported
  contracts.

## Proof

Passed delegated proof:

- `cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`
- `test_after.log` shows all 3 selected tests passed:
  `backend_prepared_lookup_helper`,
  `backend_aarch64_branch_control_lowering`, and
  `backend_aarch64_instruction_dispatch`.
