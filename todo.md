Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Local Helper Duplication

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution scratchpad for
Step 1.

## Suggested Next

Begin Step 1 by inventorying repeated AArch64-local helper shapes across
`globals.*`, `atomics.*`, `cast_ops.*`, `memory.*`, and the existing local
utility owners named in `plan.md`.

## Watchouts

Keep the first packet inventory-only unless the supervisor delegates a bounded
implementation slice. Do not move target-local register spelling, scratch
selection, or owner-specific lowering decisions into shared BIR or prealloc
authority.

## Proof

Not run; lifecycle-only activation.
