Status: Active
Source Idea Path: ideas/open/233_phase_e3_route7_materialized_condition_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Acceptance Notes

# Current Packet

## Just Finished

Completed `plan.md` Step 3 as a todo-only proof slice. I reviewed the Step 2
helper-oracle assertions against the nearby same-feature branch-control and
instruction-dispatch tests and found no concrete missing assertion that justifies
touching test code in this packet.

Coverage recorded:

- Positive Route 7/BIR/prepared agreement: covered locally by
  `verify_prepared_bir_comparison_condition_producer_equivalence()` requiring
  `route7_find_materialized_condition(...)`,
  `route_index_validate_materialized_condition_reference(...)`,
  `find_materialized_condition_producer_identity(...)`, and
  `find_prepared_materialized_condition_producer(...)` to agree for `%cond`;
  covered nearby in
  `materialized_compare_branch_route7_provenance_matches_bir_identity()`.
- Absent-route fallback: covered locally by the empty
  `Route7ComparisonConditionIndex`/`RouteIndexReferenceFacade` checks and nearby
  by `materialized_compare_branch_absent_route7_provenance_uses_emitted_fallback()`.
- Invalid-reference fallback: covered locally by the stale/null instruction
  Route 7 record check and nearby by
  `materialized_compare_branch_invalid_route7_reference_rejected()`.
- Duplicate/conflict fallback: covered locally by the duplicate `%cond`
  materialized-condition record check and nearby by
  `materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback()`.
- Mismatch fallback: covered locally by Route 7/prepared producer mismatch and
  missing lhs producer evidence; covered nearby by condition-name mismatch,
  stale prepared lookup, lhs provenance mismatch, and rhs provenance mismatch
  branch-control cases.
- Unfused/non-comparison fallback: covered locally by the non-comparison
  materialized-condition lookup and nearby by fused-compare fallback coverage in
  `public_selected_fused_compare_operand_producer_fallbacks_keep_prepared_row()`.
- Prepared fallback/non-agreement: covered locally by requiring the local
  agreement predicate to return false while prepared/BIR facts remain available;
  covered nearby by stale prepared lookup and lhs/rhs provenance mismatch
  branch-control cases.
- Same-feature stability: covered by the delegated
  `backend_aarch64_branch_control_lowering` same-feature materialized-compare
  cases plus `backend_aarch64_instruction_dispatch`, including
  `materialized_compare_branch_reuses_emitted_latch_operand()`.

No test edit was made because Step 2's local helper-oracle assertions already
cover the requested matrix and the delegated nearby tests cover production
branch-control/instruction-dispatch stability without output-contract changes.

## Suggested Next

Delegate Step 4 to validate and prepare acceptance notes.

Suggested packet:

- Run the supervisor-selected acceptance validation, compare against
  `test_before.log` if requested by regression-guard workflow, and summarize the
  retained prepared authority plus unchanged output/string surfaces.

## Watchouts

- Step 3 found no remaining local proof gap for positive agreement, absent-route,
  invalid-reference, duplicate/conflict, mismatch, unfused/non-comparison,
  prepared fallback/non-agreement, or nearby same-feature stability within the
  delegated three-test proof scope.
- Step 4 acceptance notes should still explicitly state that no helper-oracle
  strings, expected strings, baselines, branch-control output, machine-printer
  output, wrappers, final assembler behavior, route-index public contracts, or
  supported/unsupported contracts changed.
- If the supervisor wants higher confidence, broaden validation rather than
  adding testcase-shaped assertions to the selected helper row.

## Proof

Passed delegated proof:

- `cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`
- `test_after.log` shows all 3 selected tests passed:
  `backend_prepared_lookup_helper`,
  `backend_aarch64_branch_control_lowering`, and
  `backend_aarch64_instruction_dispatch`.
