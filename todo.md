Status: Active
Source Idea Path: ideas/open/241_f128_full_width_constant_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current Constant Payload Authority

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1. No implementation
packet has run yet.

## Suggested Next

Delegate Step 1 to an executor with a narrow proof command that covers current
F128 constant fail-closed behavior and existing scalar immediate behavior.

## Watchouts

- Do not claim F128 constant progress through `F64`, `immediate`,
  `immediate_bits`, `double`, or any single-lane 64-bit payload.
- Do not add AArch64 constant assembly printing before a structured full-width
  constant carrier exists and reaches selection.
- Keep arithmetic, comparison, cast, helper-call, atomic, intrinsic, inline
  assembly, scalar FP, and i128 routes out of this plan unless the source idea
  explicitly requires a shared carrier-preserving touch.

## Proof

Lifecycle-only activation; no build or test proof was required.
