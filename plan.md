# AArch64 Callee-Saved Scalar Home Preservation Runbook

Status: Active
Source Idea: ideas/open/337_aarch64_callee_saved_scalar_home_preservation.md

## Purpose

Follow the residual exposed after closing the AArch64 return-publication owner:
generated code can keep a live scalar value in a callee-saved physical register
across a call without adding that register to the frame preservation set.

## Goal

Repair the general AArch64 callee-saved scalar-home preservation rule without
matching `00168`, `factorial`, recursion, or literal register strings.

## Core Rule

Do not fix this through return-move suppression or testcase-shaped handling.
Localize the semantic owner that decides whether a scalar home in a
callee-saved physical register is preserved across calls, then repair that
owner generally.

## Read First

- `ideas/open/337_aarch64_callee_saved_scalar_home_preservation.md`
- `ideas/closed/336_aarch64_return_result_publication_epilogue_clobber.md`
- current `todo.md`
- generated assembly for `build/c_testsuite_aarch64_backend/src/00168.c.s`
- prepared/backend dumps for the recursive `factorial` function, especially
  scalar homes, call liveness, frame register preservation, and prologue /
  epilogue emission

## Current Targets

- `c_testsuite_aarch64_backend_src_00168_c`, where `factorial` keeps caller
  `n` in `w19` across `bl factorial` but the generated frame omits `x19`.
- Focused backend coverage for a scalar home assigned to a callee-saved
  physical register and live across a call.
- Adjacent AArch64 frame, call-boundary, scalar-home, and return-publication
  guardrails selected by the supervisor.

## Non-Goals

- Do not reopen the return-publication repair from idea 336 unless new
  generated-code evidence shows the stale terminal overwrite has returned.
- Do not match `00168`, `factorial`, recursion, `w19`/`x19`, source lines, or
  the exact multiplication instruction sequence.
- Do not edit expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, or CTest registration.
- Do not broad-rewrite AArch64 register allocation, frame layout, prologue /
  epilogue, or call lowering without localization proving the narrow owner is
  insufficient.

## Working Model

The remaining `00168` failure is not that `w0` loses return authority. The
recursive return result reaches `w0`; the caller then multiplies it by a scalar
home that should still hold the caller's `n`. That home lives in `w19` across
the recursive call, but the frame does not preserve `x19`, so the callee's
frame leaves the caller's home clobbered.

## Execution Rules

- Start from generated assembly and backend/prepared dumps before changing
  code.
- Identify whether the missing fact is in live-range analysis, physical
  assignment, frame planning, prologue/epilogue emission, or a handoff between
  those layers.
- Prefer a repair that records only actually used/live callee-saved physical
  registers, not unconditional preservation of the whole callee-saved set.
- Add focused backend coverage before relying on `00168` as the only proof.
- Keep proof logs in the canonical files selected by the supervisor.

## Ordered Steps

### Step 1: Localize Missing Callee-Saved Preservation

Goal: identify where the live scalar home in a callee-saved register stops
being represented in frame preservation data.

Primary targets: generated `00168` assembly, prepared dumps, frame plan data,
and AArch64 prologue/epilogue inputs.

Actions:

- Inspect the generated `factorial` body and record the lifetime of caller
  `n` through the recursive `bl factorial`.
- Trace the assignment or home selection that places the live scalar in
  `w19`/`x19`.
- Compare that assignment against the frame's saved/restored register set.
- Decide whether the owner is liveness, register assignment, frame planning,
  prologue/epilogue emission, or a handoff between them.

Completion check:

- `todo.md` records the first point where the live callee-saved scalar home is
  dropped from preservation authority, with concrete evidence from `00168` and
  at least one focused backend dump or record.

### Step 2: Repair The Preservation Owner

Goal: ensure live scalar homes in callee-saved physical registers are preserved
across calls according to the backend's intended frame contract.

Primary target: the localized backend component from Step 1.

Actions:

- Apply the smallest semantic repair to carry used/live callee-saved register
  facts into frame preservation or to avoid assigning such unpreserved homes.
- Keep the repair independent of testcase names and literal register strings.
- Preserve the return-publication behavior fixed by idea 336.
- Rebuild or compile the touched target as required by the supervisor packet.

Completion check:

- Focused generated output or backend records show the live callee-saved home
  is either saved/restored by the frame or no longer lives in an unpreserved
  callee-saved register across the call.

### Step 3: Add Focused Backend Coverage

Goal: lock the repaired preservation rule into a local backend test.

Primary target: AArch64 backend tests covering frame/register preservation and
scalar-home call liveness.

Actions:

- Add or update focused backend coverage that fails without the repair.
- Assert the semantic preservation fact, not just a filename-shaped assembly
  string.
- Include a nearby guard that return-result publication still avoids stale
  terminal overwrites where practical.

Completion check:

- Focused backend coverage passes and would have failed on the missing
  callee-saved preservation fact.

### Step 4: Prove Runtime Representative And Guardrails

Goal: verify the repair on `00168` and adjacent backend behavior.

Primary target: supervisor-selected c-testsuite representative and backend
guardrail subset.

Actions:

- Run `c_testsuite_aarch64_backend_src_00168_c`.
- Run the focused backend tests touched by the repair.
- Run adjacent AArch64 frame, call-boundary, scalar-home, and
  return-publication guardrails selected by the supervisor.
- If `00168` advances to a new first bad fact, record the residual without
  expanding this plan unless it is the same preservation owner.

Completion check:

- `00168` passes or advances past missing `x19` preservation, focused backend
  coverage passes, guardrails are recorded in `todo.md`, and any residual is
  clearly classified for plan-owner review.
