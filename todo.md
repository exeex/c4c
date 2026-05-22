Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
the active runbook.

## Suggested Next

Run Step 1: rebuild, refresh the current focused representative/focused
backend evidence, and classify whether Hanoi starting-state value-flow is
still a live first bad fact.

## Watchouts

- The source idea was previously parked after scoped progress; do not assume
  the historical `A: 1 2 0 4` starting-state mismatch still exists.
- If current evidence points to materialized pointer `StoreLocal` writeback,
  pointer-derived load/address scaling, recursive post-call pointer-formal
  handling, or another owner, record the classification instead of widening
  this plan.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.

## Proof

Not run during lifecycle-only activation.
