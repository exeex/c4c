Status: Active
Source Idea Path: ideas/open/173_route7_comparison_condition_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract Only Redundant Prepared Surface

# Current Packet

## Just Finished

Plan Step 3 expanded `backend_prepared_lookup_helper_test` equivalence proof for
the migrated Route 7 fused-compare operand consumer. The prepared/BIR fixture now
has a real conditional terminator and checks Route 7 fused-operand facts at the
consumer boundary (`block.insts.size()`), matching prepared lhs select producer
and rhs folded-integer answers.

The test now poisons Route 7 indexes while the underlying BIR producer scan would
still find valid producers, proving fail-closed behavior for unavailable index,
missing lhs producer, duplicate branch reference, wrong key, wrong relationship,
and stale owner/reference cases without silently broadening into a generic BIR
scan.

## Suggested Next

Delegate Step 4 to inspect remaining prepared fused-compare helper/cache
references and contract only the redundant prepared surface no longer needed by
the migrated AArch64 consumer. Keep compatibility helpers used by other
comparison/branch consumers.

## Watchouts

- Step 3 now covers prepared-oracle equivalence at the migrated consumer
  boundary and fail-closed invalid Route 7 references without broad BIR fallback.
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
- Step 4 should not remove prepared helpers still used as oracles or by
  unexpanded comparison/branch consumers.
- Treat helper renames, expectation rewrites, or named-case-only fixes as
  non-progress.

## Proof

Step 3 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_aarch64_instruction_dispatch` and
`backend_prepared_lookup_helper` both passed.
