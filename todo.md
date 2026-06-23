Status: Active
Source Idea Path: ideas/open/319_rv64_stack_homed_fused_compare_control_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Stack-Homed Fused Compare Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00077.c` and `src/00143.c`, capturing
BIR/prepared-BIR/emitted RV64/link/qemu evidence, and classifying the first
branch or loop-condition failure for each candidate.

## Watchouts

- Do not special-case filenames, SSA names such as `%t5` or `%t1`, or observed
  stack offsets.
- Do not repair later local-array/select behavior before proving the first
  stack-homed compare branch or loop condition.
- Do not fold nested select-chain/store-source, aggregate/byval,
  function-pointer, or external-call work into this route.

## Proof

Lifecycle-only activation. No build or tests were run.
