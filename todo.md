Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect I128 Prepared And AArch64 Surfaces

# Current Packet

## Just Finished

Activated `ideas/open/236_aarch64_i128_pair_lowering.md` as the active
lifecycle runbook.

## Suggested Next

Execute Step 1 from `plan.md`: inspect i128 prepared carriers, AArch64
dispatch/record/printer surfaces, and runtime helper boundaries, then record
the first implementation packet target and focused proof subset before code
changes.

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer low/high pair homes from rendered register names or fixed
  `x0`/`x1` accumulator conventions.
- Do not lower i128 as scalar i64 or claim progress through named testcase
  shortcuts.
- If prepared low/high or memory-backed carrier authority is missing, stop and
  split the exact prepared/shared blocker instead of filling it in target
  lowering.

## Proof

Lifecycle activation only. No build or test proof was required.
