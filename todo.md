Status: Active
Source Idea Path: ideas/open/342_aarch64_duff_do_while_latch_condition_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Duplicate Latch Decrement Boundary

# Current Packet

## Just Finished

Lifecycle switched from completed idea 341 to the focused Duff do-while latch
condition residual split as idea 342.

## Suggested Next

Execute Step 1: reproduce the `00143` runtime residual, regenerate prepared BIR
and AArch64 assembly probes, and localize where the second latch decrement is
introduced.

## Watchouts

- Do not reopen idea 341's fixed-offset fallthrough copy emission unless the
  old empty fallthrough copy labels return.
- Do not special-case `00143`, `.LBB1_29`, Duff-device labels, local name `n`,
  source lines, block numbers, or emitted instruction spellings.
- Keep expectation, unsupported, runner, timeout, CTest registration, and
  proof-log policy unchanged.

## Proof

Lifecycle-only switch. No implementation validation was run for idea 342.
