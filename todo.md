Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current F128 Prepared Coverage

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1. No implementation
packet has run yet.

## Suggested Next

Delegate Step 1 to an executor with a narrow proof command that covers AArch64
prepared records and current F128 fail-closed behavior.

## Watchouts

- Do not claim binary128 progress through scalar `F64` lowering.
- Do not add testcase-shaped helper shortcuts or weaken unsupported
  expectations.
- Keep atomic, intrinsic, and inline-assembly AArch64 routes out of this plan.

## Proof

Lifecycle-only activation; no build or test proof was required.
