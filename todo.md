Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Pointer-Derived Load Scaling Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for idea 362. No implementation
work has started under this plan.

## Suggested Next

Execute plan Step 1: reproduce the focused subset, inspect `00181` semantic
BIR, prepared BIR, and generated AArch64, and record the first pointer-derived
load/address scaling bad fact that explains the post-writeback timeout.

## Watchouts

- Preserve idea 361's materialized pointer store writeback evidence.
- Preserve idea 360's correct starting state.
- Do not change expectations, timeout policy, runner behavior, or unsupported
  classifications.
- Treat `00181`-specific matching or a single emitted-instruction shortcut as
  route failure.

## Proof

Lifecycle-only activation. No build or test proof required for this edit.
