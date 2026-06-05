Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Capture Failure Shape

# Current Packet

## Just Finished

Activation created the canonical runbook and executor scratchpad for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: reproduce the two current AArch64 segfaults and
record the fresh failure shape.

## Watchouts

- Do not weaken, skip, or reclassify `00216` or `00204`.
- Do not add filename-shaped or expected-output-shaped special cases.
- Keep printer-only differences out of scope unless they prove a
  construction-time side effect.

## Proof

Lifecycle-only activation; no code validation was run.
