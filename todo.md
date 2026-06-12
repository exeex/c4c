Status: Active
Source Idea Path: ideas/open/216_route7_comparison_oracle_row.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Negative and Fallback Semantics

# Current Packet

## Just Finished

Step 3 proved negative and fallback semantics for the selected materialized-condition helper-oracle row.

Changed files:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: added a focused selected-row absent-Route-7 assertion beside the existing `%cond` materialized-condition row. The new check proves an empty Route 7 authority and missing route-index facade fail closed as `MissingBlock`/`MissingRecord` while the prepared and BIR materialized-condition facts remain available.
- `todo.md`: recorded Step 3 completion and proof.

Coverage confirmed:

- Positive selected-row Route 7 query/reference evidence remains covered in `verify_prepared_bir_comparison_condition_producer_equivalence`.
- Absent Route 7 helper authority is now covered directly for the selected row; branch-control also covers absent materialized-condition provenance falling back to emitted-condition behavior.
- Invalid/stale reference is covered by `materialized_compare_branch_invalid_route7_reference_rejected`.
- Duplicate/conflict is covered by `materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback` and the helper duplicate producer/reference checks.
- Route/prepared mismatch is covered by `materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback`.
- Unfused/prepared fallback behavior remains covered by the materialized-bool/unavailable helper checks and branch-control emitted/selected fallback tests.

## Suggested Next

Step 4 should prove expected-string and wrapper stability for the same selected materialized-condition row by rerunning the delegated branch-control and machine-printer proof, confirming no expected strings, wrapper behavior, branch suffixes, branch targets, final assembler output, helper names, or supported-path contracts changed.

## Watchouts

- Step 3 intentionally did not broaden into fused-compare row replacement, branch-policy migration, wrappers, expected strings, Route 8 code, generic route-index facades, implementation files, or unrelated tests.
- Remaining Step 4 proof needs are expected-string and wrapper stability only; no Step 3 fallback ambiguity is currently open for the selected materialized-condition row.
- Expected-string rewrites, unsupported downgrades, weaker contracts, and testcase-shaped oracle handling remain blockers.

## Proof

Passed:

`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer)$' --output-on-failure | tee test_after.log`

Proof log: `test_after.log`.
