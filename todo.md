Status: Active
Source Idea Path: ideas/open/173_route7_comparison_condition_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Equivalence Against Prepared Oracles

# Current Packet

## Just Finished

Plan Step 2 added a Route 7-backed fused-compare operand-producer facts path:
`bir::route7_find_fused_compare_operand_producer_facts(...)` validates lhs/rhs
operand records through the Route 7 route-index facade, and uses the validated
Route 7 branch-condition record for terminator-index fused-branch queries.

`aarch64::codegen::fused_compare_uses_selected_operand(...)` now builds the
narrow Route 7 comparison-condition index for the current BIR block and prefers
that semantic producer answer. The existing
`bir::find_fused_compare_operand_producer_facts(...)` lookup remains as the
prepared/BIR oracle fallback. Target branch spelling, fused legality,
condition-code selection, hazards, emitted-register state, and final
instruction records stayed in AArch64 code.

Coverage in `backend_prepared_lookup_helper_test` now checks Route 7 fused
operand facts against prepared/BIR answers for lhs select producers, rhs folded
integer constants, actual immediate operands, rhs-only availability, production
prealloc branch conditions, and wrong-key/wrong-role fail-closed validation.

## Suggested Next

Delegate Step 3 to expand equivalence proof around the migrated consumer:
compare Route 7 answers with the prepared fused-operand oracle for the selected
AArch64 path, including unavailable, missing-producer, duplicate-reference, and
wrong-relationship cases that should fail closed rather than falling back to a
broad BIR scan.

## Watchouts

- Step 2 already covers lhs select producers, folded constants, actual
  immediate operands, rhs-only availability, production branch-condition
  operands, and wrong-key/wrong-role validation.
- Remaining Step 3 proof should focus on prepared-oracle equivalence at the
  migrated AArch64 consumer boundary and on invalid Route 7 references that
  must not silently widen into broad BIR scans.
- `route7_find_fused_compare_operand_producer_facts(...)` intentionally uses
  the branch-condition validation path only for terminator-index fused-branch
  queries; exact comparison operand records stay keyed by the compare
  instruction index.
- Target subset to protect the consumer behavior:
  `backend_aarch64_instruction_dispatch`, especially
  `selected_global_load_materializes_before_fused_compare_branch()` and
  `rhs_selected_global_load_materializes_before_fused_compare_branch()`.
- Keep target branch spelling, fused-compare legality, condition-code
  selection, hazards, emitted-register state, and final instruction records out
  of BIR.
- Do not contract prepared comparison helpers before the selected consumer has
  Route 7-backed equivalence proof.
- Treat helper renames, expectation rewrites, or named-case-only fixes as
  non-progress.

## Proof

Step 2 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_aarch64_instruction_dispatch` and
`backend_prepared_lookup_helper` both passed.
