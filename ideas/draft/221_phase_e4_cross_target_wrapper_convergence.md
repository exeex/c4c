# 221 Phase E4 cross-target wrapper convergence

## Goal

Extend already-proven BIR route facts or retained-owner adapters across x86
and riscv wrapper boundaries without moving target-local ABI, frame, register,
formatting, instruction-selection, or emission policy into BIR.

This phase is one wrapper input or diagnostic row at a time.

## Prerequisite

Do not open this draft until E0 names cross-target wrapper candidates and the
corresponding AArch64 route fact or retained-owner boundary is already proven.

## Direction

Cross-target convergence means shared semantic input, not shared target policy.
Each slice must prove target-local fallback and output stability.

## In Scope

- x86 or riscv consumption of one already-proven route fact.
- One wrapper/debug row that can be checked without broad target migration.
- Compatibility adapters that keep prepared fallback for absent, mismatched,
  invalid, duplicate/conflict, or unsupported target-local cases.

## Out Of Scope

- Route-wide x86/riscv migration.
- Inventing target-only BIR facts that bypass AArch64-proven semantics.
- Moving frame/register/ABI/layout/formatting/emission policy into BIR.
- Broad wrapper-family contraction.

## Expected Output

The closure note must contain:

- the exact target wrapper or debug row touched;
- the already-proven route fact or retained-owner adapter consumed;
- retained target-local policy and prepared fallback boundaries;
- proof for positive, absent, invalid, duplicate/conflict, mismatch, fallback,
  wrapper output, and expected strings;
- any remaining cross-target blockers before E5.

## Reviewer Reject Signals

- Treating one x86 Route 6 proof as route-wide cross-target readiness.
- Creating riscv-only BIR adapters without shared semantic ownership.
- Weakening wrapper tests or expected strings to make convergence appear green.
