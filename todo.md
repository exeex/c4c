# Current Packet

Status: Active
Source Idea Path: ideas/open/129_aarch64_i128_shift_support_completeness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Shift Contract And Coverage

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/129_aarch64_i128_shift_support_completeness.md`.

## Suggested Next

Execute Step 1 by mapping current AArch64 i128 shift lowering, printing, and
backend coverage for shift-by-64-or-more and variable-count shifts.

## Watchouts

- Keep the route target-local to AArch64 i128 shifts.
- Do not weaken supported-path expectations or add named-testcase shortcuts.
- Do not expand into f128, helper ABI/resource policy, carrier ownership, or
  shared BIR/prealloc policy.

## Proof

Lifecycle-only activation. No build or test proof required yet.
