# AArch64 Dynamic Stack VLA Fixed Slot Addressing Runbook

Status: Active
Source Idea: ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md
Supersedes: ideas/open/295_backend_regex_failure_family_inventory.md Step 4 timeout split

## Purpose

Repair the `00207` generated-program runtime timeout by fixing the general
AArch64 dynamic stack/VLA lowering rule that leaves fixed stack homes addressed
relative to a moving `sp`.

## Goal

Make `c_testsuite_aarch64_backend_src_00207_c` advance beyond the repeated
`boom!` runtime timeout or pass under existing runner and timeout policy.

## Core Rule

Progress must repair the semantic AArch64 frame-lowering/addressing rule for
fixed stack homes across dynamic stack allocation. Do not claim progress
through timeout policy, runner behavior, expectation weakening,
unsupported-list changes, CTest registration changes, proof-log policy, or
filename-specific shortcuts.

## Read First

- `ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `tests/c/external/c-testsuite/src/00207.c`
- Current `test_before.log` / `test_after.log` only if the supervisor says
  they are the active baseline artifacts.
- Existing `build/c_testsuite_aarch64_backend/src/00207.c.s` and
  `build/c_testsuite_aarch64_backend/src/00207.c.bin` only if freshly
  regenerated for the delegated proof.

## Current Target

- `c_testsuite_aarch64_backend_src_00207_c`

## Non-Goals

- Do not work on `00200` shift/type-promotion backend asm-generation
  scalability under this plan; that owner is closed in
  `ideas/closed/381_aarch64_shift_promotion_codegen_scalability_timeout.md`.
- Do not treat this as backend compile/codegen slowness after fresh proof has
  shown asm and binary emission completed before timeout.
- Do not broaden into general VLA semantic admission work unrelated to fixed
  homes addressed relative to a moving `sp`.
- Do not broaden into ABI composite, variadic, HFA, frontend admission, or
  unrelated runtime output buckets.
- Do not change tests, expectations, unsupported classifications, timeout
  policy, runners, CTest registration, or proof-log policy.
- Do not special-case `00207`, `f1`, `boom!`, one label, one stack offset, one
  register, or one emitted instruction neighborhood.

## Working Model

- `00207` is a generated-program runtime timeout, not backend compile/codegen
  slowness.
- Fresh CTest proof has regenerated `build/c_testsuite_aarch64_backend/src/00207.c.s`
  and `build/c_testsuite_aarch64_backend/src/00207.c.bin` before the test timed
  out.
- A direct bounded binary run repeatedly printed `boom!` until timeout killed
  it, while the source expects exactly two `boom!` lines before later numeric
  output.
- The first bad fact is in `f1`: `argc` is initially spilled at `[sp, #8]`,
  then a VLA allocation moves `sp`; later loop reload/update code still uses
  stack slots such as `[sp, #8]` and `[sp, #24]` relative to the adjusted stack
  pointer.
- The loop-control value no longer reliably reaches zero.

## Execution Rules

- Use bounded diagnostics and stale-process cleanup for any timeout
  investigation.
- Localize the first fixed-home/moving-`sp` address before changing code.
- Compare semantic BIR, prepared BIR, frame layout, generated AArch64, and
  bounded runtime output around the `f1` loop-control value.
- Prefer focused backend coverage for fixed stack homes across a moving
  dynamic stack pointer before relying on the external c-testsuite case alone.
- Keep proof commands narrow during implementation, then use supervisor-chosen
  broader validation when the slice is acceptance-ready.
- Reject count-only improvement if `00207` still has the same repeated
  `boom!` timeout.

## Steps

### Step 1: Reconfirm Stage and First Bad Address

Goal: prove the current timeout is still generated-program runtime behavior
and localize the first fixed stack home addressed through the moved `sp`.

Actions:
- Run the supervisor-delegated narrow proof command for `00207` with an
  explicit timeout.
- Confirm fresh asm and binary emission complete before the runtime timeout.
- Capture bounded runtime output and generated AArch64 around the `f1`
  loop-control value.
- Compare source, semantic BIR, prepared BIR, frame layout, and generated
  addressing for the first fixed home that is still relative to adjusted `sp`.
- Record the first bad address and evidence path in `todo.md`.

Completion Check:
- `todo.md` names the first fixed-home/moving-`sp` address responsible for the
  repeated runtime output, or explains the exact ambiguity blocking
  implementation.

### Step 2: Add Focused Dynamic Stack Coverage

Goal: create a focused backend test that represents the localized
dynamic-stack/VLA fixed-slot addressing shape without depending on the `00207`
filename.

Actions:
- Extract the smallest representative shape from the localized first bad fact.
- Add or update focused backend coverage for fixed stack homes across a moving
  dynamic stack pointer.
- Verify the focused test fails before the repair for the same reason.

Completion Check:
- Focused coverage exists for the dynamic-stack fixed-slot addressing shape and
  is not a testcase-shaped shortcut.

### Step 3: Repair Fixed-Slot Addressing

Goal: fix the general AArch64 frame-lowering rule so stable fixed stack homes
are not addressed through a moving `sp` after dynamic stack allocation.

Actions:
- Apply the smallest semantic repair in the localized frame-lowering or
  addressing path.
- Preserve non-dynamic stack frame-slot publication and call-boundary behavior.
- Avoid broad rewrites unless localization proves they are required.

Completion Check:
- Focused coverage passes and the repair does not rely on filename, source
  label, stack offset, one register, emitted instruction neighborhood,
  timeout-policy, runner, expectation, unsupported-list, or CTest-registration
  changes.

### Step 4: Prove External Advancement and Guard Neighbors

Goal: demonstrate `00207` advances beyond the repeated `boom!` timeout or
passes, then check relevant neighbors.

Actions:
- Run the supervisor-delegated narrow `00207` proof command.
- Run focused backend tests covering dynamic-stack fixed-slot addressing and
  related AArch64 frame behavior.
- If `00207` advances to a separate runtime return-status or output issue,
  record that as an independent residual instead of folding it into this
  owner.

Completion Check:
- `test_after.log` or the delegated proof artifact shows `00207` no longer has
  the repeated `boom!` timeout, or `todo.md` records the blocking evidence.

### Step 5: Acceptance Handoff

Goal: leave the slice ready for supervisor review, broader validation, and
commit decision.

Actions:
- Summarize changed files, proof commands, and any remaining residual in
  `todo.md`.
- Call out that closed idea 381 remains untouched.
- Request supervisor-side broader validation if the repair changes shared
  backend frame lowering, stack layout, or emission behavior.

Completion Check:
- The focused owner is ready for supervisor acceptance or has a clear blocker
  that can be routed without reopening umbrella idea 295.
