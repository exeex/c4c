Status: Active
Source Idea Path: ideas/open/215_route7_comparison_provenance_consumer.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Negative and Fallback Cases

# Current Packet

## Just Finished

Step 3 proved negative and fallback coverage for exactly the selected
consumer, `lower_materialized_compare_condition_branch`, after the Step 2
route/prepared agreement change.

Changed files:

- `todo.md`

Coverage confirmed:

- Positive route/prepared agreement:
  `materialized_compare_branch_route7_provenance_matches_bir_identity` proves
  Route 7, generic BIR identity, and prepared producer agreement before
  selected lowering.
- Absent Route 7:
  `materialized_compare_branch_absent_route7_provenance_uses_emitted_fallback`
  proves consumer-reachable emitted-condition fallback (`cbnz`, no `cmp`).
- Duplicate/conflicting Route 7 facts:
  `materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback`
  proves duplicate materialized-condition provenance is rejected and preserves
  emitted-condition fallback.
- Condition mismatch:
  `materialized_compare_branch_condition_name_mismatch_uses_emitted_fallback`
  proves mismatched condition identity preserves emitted-condition fallback.
- Route/prepared mismatch:
  `materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback` proves
  stale prepared lookup disagreement preserves selected generic BIR compare
  fallback (`cmp`/conditional branch, no `cbnz`).
- Operand mismatch:
  `materialized_compare_branch_lhs_provenance_mismatch_uses_emitted_fallback`
  and `materialized_compare_branch_rhs_provenance_mismatch_uses_emitted_fallback`
  prove selected BIR fallback for lhs and rhs provenance disagreement.
- Invalid/stale Route 7 reference handling:
  `materialized_compare_branch_invalid_route7_reference_rejected` covers the
  helper-level stale-owner rejection. This remains non-injectable for the
  selected consumer because `lower_materialized_compare_condition_branch`
  builds its Route 7 index internally from the current block immediately before
  validation; injecting a mutated stale-owner index would broaden beyond the
  selected consumer packet.

No new assertion was needed: the current branch-control/helper coverage already
separates consumer-reachable emitted fallback, selected generic BIR fallback,
and helper-level invalid/stale reference rejection without weakening expected
strings.

## Suggested Next

Proceed to Step 4: Prove Expected-String Stability. Keep the next packet
limited to confirming branch-control and machine-printer expected strings are
unchanged under the selected Route 7 provenance consumer change.

## Watchouts

- Step 4 still needs explicit expected-string stability proof; this Step 3
  packet intentionally did not edit branch-control or machine-printer expected
  strings.
- Do not expand Step 4 into fused compare consumers, Route 8, generic route
  facades, branch policy, suffix/target ownership, machine-printer expected
  strings, or final assembler baseline rewrites.

## Proof

Fresh delegated proof passed and wrote `test_after.log`:

```sh
cmake --build build --target backend_aarch64_branch_control_lowering_test backend_prepared_lookup_helper_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper|backend_aarch64_machine_printer)$' --output-on-failure | tee test_after.log
```

Result: 3/3 tests passed (`backend_aarch64_branch_control_lowering`,
`backend_prepared_lookup_helper`, and `backend_aarch64_machine_printer`).
