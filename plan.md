# AArch64 MOVI Zero-Extension Refresh Runbook

Status: Active
Source Idea: ideas/open/332_aarch64_movi_zero_extension_materialization.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and resolve the still-open AArch64 MOVI zero-extension materialization
owner without reopening adjacent HFA/floating, byval aggregate, stdarg cursor,
fixed-formal, or OPI result-publication repairs.

## Goal

Confirm whether a current first bad fact still exists where AArch64 MOVI or
integer immediate materialization sign-extends a value that C output expects
to be zero-extended, then either close the idea or hand off a freshly
localized residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
general MOVI or integer materialization path converting an expected
zero-extended value into a sign-extended value.

## Read First

- `ideas/open/332_aarch64_movi_zero_extension_materialization.md`
- The source idea's lifecycle handoff, especially the old `00204.c` MOVI
  mismatch examples `abcd0000`, `aaaaaaaa`, and `f8f8f8f8`.
- Adjacent AArch64 variadic and integer-publication ideas named by the source
  idea: HFA/floating publication, byval aggregate lane publication, stdarg
  cursor progression, fixed-formal entry publication, local/value-home
  publication, frame/formal publication, and OPI pointer/integer result
  publication.
- Generated BIR, prepared BIR, and AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00204_c` only as representative evidence,
  not as a testcase-specific repair target.

## Current Targets

- AArch64 MOVI and integer immediate materialization where source width,
  extension opcode, or register width can affect zero-extension.
- BIR scalar immediate `SExt`, `ZExt`, and `Trunc` cast folding through the
  integer cast helper.
- Focused backend coverage for MOVI zero-extension or adjacent integer
  materialization behavior.
- `c_testsuite_aarch64_backend_src_00204_c` as an external representative
  after focused owner evidence is checked.

## Non-Goals

- Do not reopen HFA/floating, byval aggregate lane publication, aggregate
  `va_arg`, stdarg cursor progression, fixed-formal entry publication,
  local/value-home publication, frame/formal publication, large frame layout,
  or OPI result-publication work unless fresh generated-code evidence proves a
  lifecycle split is needed.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Do not special-case `00204.c`, the MOVI block, `abcd0000`, `aaaaaaaa`,
  `f8f8f8f8`, one output line, one register, one stack slot, or one emitted
  instruction sequence.

## Working Model

The source idea's original failure was a MOVI integer materialization mismatch
where high-bit values were printed as sign-extended 64-bit values instead of
zero-extended payloads. The lifecycle handoff says commit `9ad547218`
repaired that owner by folding BIR scalar immediate `SExt`, `ZExt`, and
`Trunc` casts through the integer cast helper, and the remaining `00204.c`
failure moved later to OPI pointer/integer result publication. This runbook
therefore starts with a refresh and only proceeds to implementation if the
MOVI zero-extension owner is live again.

## Execution Rules

- Treat `ideas/open/332_aarch64_movi_zero_extension_materialization.md` as the
  durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If focused MOVI coverage and the representative remain green, request
  lifecycle closure instead of implementation work.
- If the representative fails for an out-of-scope owner, record the handoff at
  the lifecycle layer and split or activate the appropriate source idea.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.
- Reject expectation downgrades, testcase-shaped matching, helper renames, or
  classification-only notes claimed as capability progress.

## Step 1: Refresh The Current MOVI Zero-Extension First Bad Fact

Goal: Determine whether the current tree still has an in-scope MOVI or
integer zero-extension materialization failure.

Primary Target: MOVI zero-extension focused coverage and the `00204.c`
representative generated artifacts.

Actions:

- Build the current tree.
- Run the focused supervisor-selected subset for MOVI zero-extension and the
  `00204.c` representative.
- If anything fails, inspect generated BIR, prepared BIR, and AArch64
  artifacts to locate the first bad fact before choosing an owner.
- Classify the first bad fact as in scope for this source idea or explicitly
  out of scope.

Completion Check:

- The current first bad fact is either absent, or classified with concrete
  generated-code evidence and an owner boundary.

## Step 2: Close Or Hand Off If The Owner Is Not Live

Goal: Avoid expanding a parked source idea when the current failure is absent
or belongs elsewhere.

Primary Target: lifecycle state for this active plan.

Actions:

- If Step 1 is green and no in-scope first bad fact remains, request
  plan-owner close using matching focused before/after logs or the current
  supervisor-approved closure mode.
- If Step 1 finds an out-of-scope first bad fact, write the lifecycle handoff
  at the lowest correct layer and split or activate the appropriate separate
  source idea.
- Do not modify implementation files in this step.

Completion Check:

- The idea is closed, or lifecycle state is redirected to the correct
  out-of-scope owner without mutating this source idea's scope.

## Step 3: Repair Only A Returned In-Scope Owner

Goal: Implement a bounded repair only if Step 1 proves this source idea is
again the active owner.

Primary Target: the concrete AArch64 MOVI or integer materialization path
localized by Step 1.

Actions:

- Delegate a narrow executor packet naming the localized owner, target files,
  focused proof command, and adjacent guardrails.
- Add or update focused backend coverage that fails without the repair and
  passes with it.
- Preserve completed adjacent repairs named in the source idea's handoff
  notes.

Completion Check:

- The focused owner coverage passes, the representative advances or passes,
  and any remaining first bad fact is reclassified before this idea is
  considered for closure.
