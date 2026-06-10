# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the prepared comparison producer oracle

## Just Finished

Lifecycle activation created the active runbook and initialized execution
state for Step 1. No implementation work has been performed for this plan.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the prepared fused-operand and
materialized-condition producer helpers, classify BIR-owned semantic producer
identity versus target-owned compare/branch policy, and record missing narrow
coverage before implementation.

## Watchouts

- Keep fused-compare legality, condition-code selection, branch emission
  strategy, final instruction records/errors, hazard handling, emitted-register
  state, and AArch64 compare/branch instruction selection outside BIR.
- Do not switch comparison or branch consumers before prepared-oracle
  equivalence is proven for the specific provenance read.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Lifecycle-only activation. No build or test proof required yet.
