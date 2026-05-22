Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
the active runbook.

## Suggested Next

Run Step 1: rebuild, refresh the current focused representative/focused
backend evidence, and classify whether scalar cast stack-homed register source
publication is still a live first bad fact.

## Watchouts

- The source idea was previously parked after scoped progress; do not assume
  the historical `00143` structured register-source diagnostic still exists.
- If current evidence points to fallthrough fixed-offset local load/store
  emission, local storage/writeback sizing, or another owner, record the
  classification instead of widening this plan.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.

## Proof

Not run during lifecycle-only activation.
