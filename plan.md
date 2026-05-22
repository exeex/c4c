# AArch64 Variadic Aggregate Va Arg Call Setup Refresh Runbook

Status: Active
Source Idea: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and resolve the still-open AArch64 post-`va_arg` ordinary call setup
owner without reopening adjacent byval aggregate, HFA/floating, fixed-formal,
or local/value-home publication repairs.

## Goal

Confirm whether a current first bad fact still exists where an ordinary call
after non-HFA aggregate `va_arg` consumption fails to publish fixed call
operands, such as a format string in `x0` and an aggregate-member pointer in a
following argument register, then either close the idea or hand off a freshly
localized residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
post-aggregate-`va_arg` ordinary call reaching `bl` without publishing its
fixed call operands into the required AAPCS64 argument registers.

## Read First

- `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`
- The source idea's lifecycle handoff, especially the old `%7s` / `%9s`
  `myprintf` callsite evidence and the split non-HFA aggregate materialization
  residual.
- Adjacent AArch64 variadic and aggregate ideas named by the source idea:
  byval aggregate lane publication, non-HFA aggregate `va_arg`
  materialization, HFA/floating residuals, fixed-formal entry publication,
  local/value-home publication, and frame/formal publication.
- Generated BIR, prepared BIR, and AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00204_c` only as representative evidence,
  not as a testcase-specific repair target.

## Current Targets

- Ordinary calls emitted after non-HFA aggregate `va_arg` consumption.
- Fixed call operands that must be published before `bl`, especially format
  string constants and aggregate-member pointer values.
- Focused backend coverage proving post-`va_arg` call operand publication.
- `c_testsuite_aarch64_backend_src_00204_c` as an external representative after
  focused owner evidence is checked.

## Non-Goals

- Do not reopen idea 328 byval aggregate lane publication unless generated
  code again shows prepared fixed aggregate call lanes failing before `bl`.
- Do not reopen non-HFA aggregate `va_arg` materialization, HFA/floating
  variadic lowering, fixed-formal entry publication, local/value-home
  publication, frame/formal publication, global initializer emission, runner
  behavior, timeout policy, expectations, unsupported classifications,
  proof-log policy, or CTest registration without fresh generated-code
  evidence and a lifecycle split.
- Do not special-case `00204.c`, `myprintf`, `%7s`, `%9s`, `.str31`, `.str33`,
  `x0 == 0x1`, one aggregate size, one stack offset, one local temporary, or
  one emitted `printf` sequence.

## Working Model

The source idea's original failure was an ordinary `printf` call after
aggregate `va_arg` consumption where generated `myprintf` could branch to
libc with `x0 == x1` instead of loading the fixed format string into `x0` and
the aggregate text-buffer pointer into `x1`. The lifecycle handoff says that
owner was repaired and the remaining representative mismatch moved to
aggregate byte materialization. This runbook therefore starts with a refresh
and only proceeds to implementation if the post-`va_arg` call setup owner is
live again.

## Execution Rules

- Treat `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md` as
  the durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If focused call-setup coverage and the representative remain green, request
  lifecycle closure instead of implementation work.
- If the representative fails for an out-of-scope owner, record the handoff at
  the lifecycle layer and split or activate the appropriate source idea.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.
- Reject expectation downgrades, testcase-shaped matching, or helper renames
  claimed as capability progress.

## Step 1: Refresh The Current Post-Va-Arg Call Setup First Bad Fact

Goal: Determine whether the current tree still has an in-scope
post-aggregate-`va_arg` ordinary call setup failure.

Primary Target: post-`va_arg` ordinary call operand publication coverage and
the `00204.c` representative generated artifacts.

Actions:

- Build the current tree.
- Run the focused supervisor-selected subset for post-`va_arg` call setup and
  the `00204.c` representative.
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

Primary Target: the concrete AArch64 post-`va_arg` ordinary call setup path
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
