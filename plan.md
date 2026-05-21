# AArch64 Shift Promotion Codegen Scalability Timeout Runbook

Status: Active
Source Idea: ideas/open/381_aarch64_shift_promotion_codegen_scalability_timeout.md
Supersedes: ideas/open/295_backend_regex_failure_family_inventory.md Step 4 timeout split

## Purpose

Repair the `00200` timeout by localizing and fixing the backend-native AArch64
asm-generation scalability problem for expanded shift/type-promotion CFGs.

## Goal

Make `c_testsuite_aarch64_backend_src_00200_c` advance beyond backend asm
generation timeout or pass under existing timeout and runner policy.

## Core Rule

Progress must be a semantic backend scalability repair for the expanded
shift/type-promotion codegen shape. Do not claim progress through timeout
policy, runner behavior, expectation weakening, unsupported-list changes,
CTest registration changes, or filename-specific shortcuts.

## Read First

- `ideas/open/381_aarch64_shift_promotion_codegen_scalability_timeout.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `tests/c/external/c-testsuite/src/00200.c`
- Current `test_before.log` / `test_after.log` only if the supervisor says
  they are the active baseline artifacts.
- Existing `build/c_testsuite_aarch64_backend/src/00200.c.s` only as a stale
  artifact; verify freshness before using it as proof.

## Current Target

- `c_testsuite_aarch64_backend_src_00200_c`

## Non-Goals

- Do not work on `00207` dynamic stack/VLA fixed-slot addressing under this
  plan; that owner is parked in
  `ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md`.
- Do not treat the stale generated binary's runtime return-status mismatch as
  the primary owner unless the backend-native timeout first advances.
- Do not change tests, expectations, unsupported classifications, timeout
  policy, runners, CTest registration, or proof-log policy.
- Do not special-case `00200`, `lshift-type.c`, one macro name, one source
  line, or one emitted instruction neighborhood.

## Working Model

- `00200` is a backend-native AArch64 asm-generation timeout.
- The source expands shift/type-promotion checks into a large straight-line
  CFG with constant-false branches.
- Semantic and prepared BIR can reach the final `printf`/`ret`; the current
  timeout is likely in prepared lowering, register allocation, or code
  emission scalability.
- A stale generated binary can run quickly, so runtime looping is not the
  current first bad fact.

## Execution Rules

- Use bounded diagnostics and stale-process cleanup for any timeout
  investigation.
- Localize the first slow backend phase before changing code.
- Prefer focused backend coverage for the localized shape before relying on
  the external c-testsuite case alone.
- Keep proof commands narrow during implementation, then use supervisor-chosen
  broader validation when the slice is acceptance-ready.
- Reject count-only improvement if `00200` still times out in the same backend
  phase.

## Steps

### Step 1: Reconfirm Stage and First Slow Phase

Goal: prove the current timeout is still backend-native asm generation and
identify the first backend phase that crosses the timeout window.

Actions:
- Run the supervisor-delegated narrow proof command for `00200` with an
  explicit timeout.
- Capture bounded diagnostics for semantic BIR, prepared BIR, machine
  preparation, register allocation, and final AArch64 emission as available.
- Separate stale generated-artifact behavior from fresh codegen proof.
- Record the first slow phase and the evidence path in `todo.md`.

Completion Check:
- `todo.md` names the first backend phase and operation family responsible for
  the timeout, or explains the exact ambiguity blocking implementation.

### Step 2: Add Focused Scalability Coverage

Goal: create a focused backend test that represents the localized
shift/type-promotion scalability shape without depending on the `00200`
filename.

Actions:
- Extract the smallest representative shape from the localized first bad fact.
- Add or update focused backend coverage for the expanded
  shift/type-promotion CFG/codegen path.
- Verify the focused test fails before the repair for the same reason.

Completion Check:
- Focused coverage exists for the semantic scalability shape and is not a
  testcase-shaped shortcut.

### Step 3: Repair the Backend Scalability Owner

Goal: fix the general AArch64 backend behavior that causes pathological
codegen time for the localized shift/type-promotion CFG.

Actions:
- Apply the smallest semantic repair in the localized backend phase.
- Preserve scalar shift, type-promotion, compare, branch, and call-return
  behavior.
- Avoid broad rewrites unless localization proves they are required.

Completion Check:
- Focused coverage passes and the repair does not rely on filename,
  timeout-policy, runner, expectation, unsupported-list, or CTest-registration
  changes.

### Step 4: Prove External Advancement and Guard Neighbors

Goal: demonstrate `00200` advances beyond the backend asm-generation timeout
or passes, then check relevant neighbors.

Actions:
- Run the supervisor-delegated narrow `00200` proof command.
- Run focused backend tests covering scalar shift/type-promotion and related
  AArch64 codegen behavior.
- If `00200` advances to a runtime return-status or output issue, record that
  as a separate residual instead of folding it into this owner.

Completion Check:
- `test_after.log` or the delegated proof artifact shows `00200` no longer
  times out in backend asm generation, or `todo.md` records the blocking
  evidence.

### Step 5: Acceptance Handoff

Goal: leave the slice ready for supervisor review, broader validation, and
commit decision.

Actions:
- Summarize changed files, proof commands, and any remaining residual in
  `todo.md`.
- Call out whether parked idea 382 remains untouched.
- Request supervisor-side broader validation if the repair changes shared
  backend lowering, register allocation, or emission behavior.

Completion Check:
- The focused owner is ready for supervisor acceptance or has a clear blocker
  that can be routed without reopening umbrella idea 295.
