Status: Active
Source Idea Path: ideas/open/173_route7_comparison_condition_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Validation

# Current Packet

## Just Finished

Plan Step 4 inspected the remaining prepared fused-compare helper/cache surface
after the selected-operand consumer migration. No code contraction was safe:
`fused_compare_uses_selected_operand(...)` already uses Route 7/BIR producer
facts directly, while the remaining prepared helper still feeds prepared
conditional branch records for compare operand homes and encodable folded RHS
constants, and `backend_prepared_lookup_helper_test` still uses it as the
prepared/BIR/Route 7 oracle.

## Suggested Next

Delegate Step 5 acceptance validation for the migrated Route 7 fused-compare
selected-operand route, using the narrow prepared lookup helper and AArch64
instruction dispatch subset before deciding whether broader backend validation
is needed.

## Watchouts

- Step 4 deliberately left `find_prepared_fused_compare_operand_producer_facts`
  intact because it remains the prepared oracle/fallback for conditional branch
  record construction and prepared/BIR/Route 7 equivalence tests.
- The selected-operand consumer remains migrated away from prepared operand
  facts; it chooses Route 7 facts first and only uses the raw BIR producer scan
  as the fallback for unavailable Route 7 facts.
- Keep target branch spelling, fused-compare legality, condition-code
  selection, hazards, emitted-register state, and final instruction records out
  of BIR.
- Treat helper renames, expectation rewrites, or named-case-only fixes as
  non-progress.

## Proof

Step 4 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1
```

Result: `backend_aarch64_instruction_dispatch` and
`backend_prepared_lookup_helper` both passed.
