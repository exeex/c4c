# AArch64 Fixed Formal Entry Publication Refresh Runbook

Status: Active
Source Idea: ideas/open/327_aarch64_fixed_formal_entry_publication.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and resolve the still-open AArch64 fixed-formal entry publication
owner without reopening adjacent variadic, byval, HFA, or local publication
repairs.

## Goal

Confirm whether a current first bad fact still exists where AArch64 callee
entry fails to publish incoming fixed formals from their AAPCS64 argument
locations into prepared homes before first use, then either close the idea or
hand off a freshly localized residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
fixed formal reaches first use before the incoming ABI argument value is
published into its prepared home.

## Read First

- `ideas/open/327_aarch64_fixed_formal_entry_publication.md`
- Parked handoff notes in related AArch64 variadic ideas named by the source
  idea, especially HFA/floating, byval aggregate lane publication,
  local/value-home publication, frame/formal publication, and aggregate helper
  lowering.
- Existing focused backend coverage for callee-side fixed formal publication.
- Generated BIR, prepared BIR, and AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00204_c` only as representative evidence,
  not as a testcase-specific repair target.

## Current Targets

- Fixed function parameters whose prepared homes differ from incoming AAPCS64
  argument locations.
- Ordinary fixed formals in variadic callees, including pointer formals such as
  `myprintf(ptr %format, ...)`.
- Focused backend coverage that proves callee-entry publication into prepared
  register or stack homes.
- `c_testsuite_aarch64_backend_src_00204_c` as an external representative after
  focused owner evidence is checked.

## Non-Goals

- Do not reopen HFA/floating `va_arg`, byval aggregate lane publication,
  global initializer emission, fixed HFA argument/return lanes,
  local/value-home publication, `va_start` destination publication, aggregate
  helper text lowering, F128 transport, scalar ALU immediate materialization,
  large frame adjustment, or stack-slot spelling without fresh generated-code
  evidence and a lifecycle split.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Do not special-case `00204.c`, `myprintf`, `%p.format`, `.str49`, `x0`,
  `x13`, one register assignment, one pointer type, one local cursor, one
  stack slot, or one emitted instruction sequence.

## Working Model

The source idea's original failure was callee entry consuming an unpublished
fixed formal from a prepared home such as `x13` while the callsite correctly
passed the argument in an AAPCS64 incoming register such as `x0`. The handoff
note says that owner was repaired and the representative moved to caller-side
byval aggregate publication. This runbook therefore starts with a refresh and
only proceeds to implementation if the fixed-formal entry publication owner is
live again.

## Execution Rules

- Treat `ideas/open/327_aarch64_fixed_formal_entry_publication.md` as the
  durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If focused fixed-formal coverage and the representative remain green, request
  lifecycle closure instead of implementation work.
- If the representative fails for an out-of-scope owner, record the handoff at
  the lifecycle layer and split or activate the appropriate source idea.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.
- Reject expectation downgrades, testcase-shaped matching, or helper renames
  claimed as capability progress.

## Step 1: Refresh The Current Fixed-Formal First Bad Fact

Goal: Determine whether the current tree still has an in-scope fixed-formal
entry publication failure.

Primary Target: callee-entry fixed-formal publication coverage and the
`00204.c` representative generated artifacts.

Actions:

- Build the current tree.
- Run the focused supervisor-selected subset for fixed-formal entry
  publication and the `00204.c` representative.
- If anything fails, inspect generated BIR, prepared BIR, and AArch64 artifacts
  to locate the first bad fact before choosing an owner.
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

- If Step 1 is green and no in-scope first bad fact remains, request plan-owner
  close using matching focused before/after logs or the current
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

Primary Target: the concrete AArch64 fixed-formal entry publication path
localized by Step 1.

Actions:

- Delegate a narrow executor packet naming the localized owner, target files,
  focused proof command, and adjacent guardrails.
- Add or update focused backend coverage that fails without the repair and
  passes with it.
- Preserve completed adjacent repairs named in the source idea's handoff notes.

Completion Check:

- The focused owner coverage passes, the representative advances or passes, and
  any remaining first bad fact is reclassified before this idea is considered
  for closure.
