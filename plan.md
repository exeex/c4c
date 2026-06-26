# RV64 Variadic Prologue Save-Area Publication Closure Runbook

Status: Active
Source Idea: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Activated from: ideas/open/391_rv64_variadic_prologue_save_area_publication.md

## Purpose

Resume idea 391 after idea 392 closed. Idea 391's save-area publication route
was previously completed, but closure was parked until the later
`va_list` expression/call-argument publication boundary was handled and the
supervisor could normalize close-gate regression logs.

## Goal

Confirm that idea 391 can close as the RV64 variadic prologue save-area
publication owner, or identify the exact remaining closure blocker.

## Core Rule

Do not reopen the save-area implementation unless fresh evidence shows the
RV64 incoming variadic GPR payloads are no longer stored into the backing area
consumed by `va_start` / `va_arg`. Treat the closed idea 392 boundary as a
separate completed owner.

## Read First

- `ideas/open/391_rv64_variadic_prologue_save_area_publication.md`
- `ideas/closed/392_rv64_va_list_expression_call_argument_value_publication.md`
- `test_before.log`
- `test_after.log`
- `test_baseline.log`
- `build/agent_state/391_step5_va-arg-13.case.log`
- `build/agent_state/391_step5_va-arg-13.c4c-disasm.log`
- `build/agent_state/391_step5_va-arg-13.clang-disasm.log`
- `build/agent_state/391_step5_va-arg-13.qemu-strace.log`
- `build/agent_state/392_postrepair_step4_va-arg-13.run.log`

## Current Targets

- Close-gate evidence for idea 391.
- Backend regression-log normalization for the current committed state.
- Representative evidence that `va-arg-13.c` no longer blocks on either idea
  391's save-area publication owner or idea 392's later call-argument
  publication owner.

## Non-Goals

- Do not edit implementation code or tests as part of this closure-readiness
  runbook.
- Do not reopen idea 392; it is closed and owns the later `va_list`
  expression/call-argument publication boundary.
- Do not broaden into same-module sret calls, EV64 `.insn.d`, or the 354
  umbrella classification work.
- Do not mark any gcc_torture case unsupported or weaken runner expectations
  to satisfy closure.

## Working Model

Idea 391's implementation evidence showed the RV64 variadic prologue storing
incoming variadic GPR payloads into the prepared backing save area. The
representative still failed then because a later `va_list` expression call
argument used the wrong payload; that later boundary became idea 392 and is now
closed. The current lifecycle task is therefore closure-readiness for idea 391,
not another implementation slice.

## Execution Rules

- Use existing proof logs when they cover the close gate; generate or normalize
  only the canonical regression logs the supervisor requires.
- Keep `test_before.log` and `test_after.log` as the canonical close-gate
  inputs.
- If the close gate passes and source acceptance is satisfied, call the plan
  owner to close 391 canonically.
- If the close gate fails, record the exact failing scope and leave 391 active
  with the next executable proof/repair step.

## Steps

### Step 1: Verify Closure Preconditions

Goal: confirm idea 391's source acceptance is satisfied in the current
committed state.

Primary target: idea 391 lifecycle note plus the closed idea 392 proof.

Actions:

- Confirm that the saved incoming variadic GPR payload publication evidence
  from idea 391 remains the accepted owner result.
- Confirm that idea 392 is closed and no longer leaves the representative on
  the later `va_list` call-argument boundary.
- Check whether `test_before.log` and `test_after.log` already cover a matching
  backend close-gate scope for the current committed state.
- Record any missing proof/log normalization requirement in `todo.md`.

Completion check: `todo.md` states whether 391 can proceed directly to
close-gate regression comparison or exactly which canonical proof artifact is
missing.

### Step 2: Normalize And Run Close-Gate Regression Guard

Goal: produce an acceptance-grade close-gate result for idea 391.

Primary target: canonical `test_before.log` and `test_after.log`.

Actions:

- If matching backend logs already exist, run the regression guard against
  them.
- If they do not match, ask the supervisor to generate matching before/after
  backend logs without touching implementation files.
- Use the same command scope on both sides; backend-only is sufficient unless
  the supervisor chooses a broader close gate.
- Record the exact guard command and result in `todo.md`.

Completion check: `todo.md` records a passing close-gate comparison, or records
the exact regression/log mismatch that blocks closure.

### Step 3: Close Or Route The Remaining Blocker

Goal: decide final lifecycle disposition for idea 391.

Primary target: `ideas/open/391_rv64_variadic_prologue_save_area_publication.md`.

Actions:

- If source acceptance and close gate both pass, call the plan owner to close
  391 canonically.
- If closure is blocked only by missing/dirty proof logs, keep this runbook
  active and record the required proof command.
- If fresh evidence shows a real save-area regression, record the exact
  failing evidence and route a narrow repair packet without expanding scope.

Completion check: idea 391 is closed canonically, or `todo.md` names the exact
remaining blocker and next executable step.
