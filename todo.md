Status: Active
Source Idea Path: ideas/open/aarch64-codegen-prepared-boundary-recovery.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Dispatch and Calls Responsibilities

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/aarch64-codegen-prepared-boundary-recovery.md`.

## Suggested Next

Execute `plan.md` Step 1: audit `dispatch_*` and `calls_*` responsibilities,
record the classification here or in `review/`, and identify one low-risk
generic Prepared/MIR responsibility to move or justify deferral.

## Watchouts

- Do not perform header-family consolidation or `.cpp` merging in this plan.
- Do not weaken tests, expectations, or backend contracts.
- Reject named-case shortcuts and target-specific special cases outside
  AArch64.

## Proof

Lifecycle-only activation; no build or test proof required yet.
