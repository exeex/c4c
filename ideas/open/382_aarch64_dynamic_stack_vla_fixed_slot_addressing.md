# AArch64 Dynamic Stack VLA Fixed Slot Addressing

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the generated-program runtime timeout caused by AArch64 dynamic stack
or VLA lowering that addresses fixed stack homes relative to a moving `sp`,
represented by `00207`.

## Why This Exists

Umbrella idea 295 classified `00207` as a generated-program runtime timeout,
not backend compile/codegen slowness. The delegated CTest proof refreshed
`build/c_testsuite_aarch64_backend/src/00207.c.s` and
`build/c_testsuite_aarch64_backend/src/00207.c.bin` before the test timed out.
A direct bounded binary run repeatedly printed `boom!` until timeout killed
it, while the source expects exactly two `boom!` lines before later numeric
output.

The first bad fact is in `f1`: `argc` is initially spilled at `[sp, #8]`, then
a VLA allocation moves `sp`; later loop reload/update code still uses stack
slots such as `[sp, #8]` and `[sp, #24]` relative to the adjusted stack
pointer. The loop control no longer reliably reaches zero.

## In Scope

- Localize fixed stack homes that remain addressed through moving `sp` across
  dynamic stack or VLA allocation.
- Compare semantic BIR, prepared BIR, frame layout, generated AArch64, and
  bounded runtime output around the `f1` loop-control value.
- Repair the general AArch64 frame-lowering rule for stable fixed-slot
  addressing when dynamic stack allocation changes `sp`.
- Add focused backend coverage for fixed stack homes across a moving dynamic
  stack pointer without encoding `00207`.
- Prove `c_testsuite_aarch64_backend_src_00207_c` advances beyond the repeated
  `boom!` timeout or passes without runner or timeout changes.

## Out Of Scope

- Backend-native asm-generation scalability for the `00200` shift/promotion
  CFG.
- General VLA semantic admission work unrelated to fixed homes addressed
  relative to a moving `sp`.
- Broad ABI composite, variadic, HFA, frontend admission, or unrelated runtime
  output buckets.
- Timeout-limit changes, runner behavior, CTest registration, unsupported
  classifications, expectation rewrites, proof-log policy, or external test
  contract changes.
- Filename-specific handling for `00207`, one stack offset, or one source
  label.

## Acceptance Criteria

- The first bad fixed-home/moving-`sp` address is localized with source,
  generated assembly, and bounded runtime evidence.
- Focused backend coverage fails before the repair and passes after it for a
  dynamic-stack/VLA fixed-slot addressing shape.
- `c_testsuite_aarch64_backend_src_00207_c` advances beyond the repeated
  `boom!` timeout or passes under the existing runner and timeout policy.
- Existing non-dynamic stack frame-slot publication and call-boundary behavior
  remain stable.
- Any separate runtime mismatch after the timeout advances is classified
  independently instead of folded into this owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00207`, `f1`, `boom!`, one label, one stack offset, one
  register, or one emitted instruction neighborhood instead of repairing a
  general dynamic-stack/VLA fixed-slot addressing rule;
- changes timeout policy, runner behavior, CTest registration, proof-log
  behavior, unsupported classifications, expectations, allowlists, or external
  test contracts;
- claims progress from pass-count movement, helper renames, classification
  notes, expectation changes, or count-only evidence while the same repeated
  `boom!` timeout remains;
- treats the issue as backend compile/codegen slowness after fresh proof has
  shown asm and binary emission completed before timeout;
- broadens into shift/type-promotion codegen scalability, ABI composite work,
  semantic BIR admission, or unrelated runtime buckets without a separate
  lifecycle split.
