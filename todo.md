Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
the active runbook.

## Suggested Next

Run Step 1: rebuild, refresh the current focused `00181` representative and
nearby pointer-derived load/address-scaling evidence, and classify whether
AArch64 pointer-derived load address scaling is still a live first bad fact.

## Watchouts

- The source idea is parked after a historical repair; do not assume the old
  post-writeback timeout still exists.
- If current evidence points to stale prepared select/join reloads,
  materialized pointer store writeback, direct `LoadGlobal` select-store
  handling, recursive formal preservation, or another owner, record the
  classification instead of widening this plan.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.

## Proof

Not run during lifecycle-only activation.
