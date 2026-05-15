Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Facts For `va_start`

# Current Packet

## Just Finished

Lifecycle transition completed: prerequisite idea 244 closed after matching
backend close-gate logs passed the monotonic regression guard, and idea 243 is
reactivated for selected machine-node consumption.

## Suggested Next

Delegate an executor packet for Step 2: consume prepared/shared facts for
`va_start` lowering. The packet should emit structured selected machine-node
records and printer output from prepared entry storage, helper-resource facts,
and `va_start` operand-home records without reconstructing AAPCS64 layout or
frame placement in AArch64 target lowering.

## Watchouts

- Step 1 is already complete; it split out prerequisite idea 244.
- Idea 244 supplied register-save-area slot/offset facts, overflow-area
  base-slot/base-offset facts, helper scratch register/stack facts, and helper
  operand-home records.
- Keep incomplete fact handling fail-closed with explicit diagnostics.
- Treat fixture-specific matching, expectation-only changes, and target-local
  ABI reconstruction as route drift.

## Proof

Plan-owner close gate for idea 244 passed:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: before=139 passed, 0 failed, 139 total; after=139 passed, 0 failed,
139 total; monotonic regression guard PASS.
