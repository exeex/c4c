# AArch64 Recursive Scalar Formal Post-Call Preservation Runbook

Status: Active
Source Idea: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md

## Purpose

Reactivate the recursive scalar-formal preservation source only long enough to
verify whether its parked first bad fact has returned. Do not assume the
historical `%p.n` post-call clobber is still current.

## Goal

Repair AArch64 scalar formal post-call preservation only if fresh generated-code
evidence shows a live scalar formal is again consumed from a clobbered ABI
argument register after an intervening call.

## Core Rule

Do not implement from stale parked evidence. Step 1 must refresh the current
first bad fact and either confirm this source idea owns a live failure or route
the plan back to lifecycle handling.

## Read First

- `ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md`
- The parked notes in that source idea, especially the note that commit
  `0a7ef5764` repaired the `%p.n` scalar formal post-call boundary and split
  the later `%p.spare` pointer residual to idea 359.
- Generated AArch64, prepared BIR, and semantic BIR artifacts for `00181` only
  as evidence, not as a testcase-specific target.

## Current Targets / Scope

- AArch64 lowering for scalar formals whose source value remains live after
  recursive or same-module calls.
- Prepared preserved homes for scalar formals such as stack homes assigned to
  values like `%p.n`.
- Post-call reload or publication logic that should use preserved scalar homes
  instead of clobbered ABI argument registers.
- Focused backend coverage for scalar formals consumed after one or more calls
  on a recursive or same-module path.
- Representative proof through `c_testsuite_aarch64_backend_src_00181_c` only
  after the scalar-formal owner is confirmed.

## Non-Goals

- Do not reopen pointer-formal callee-saved home publication from idea 357.
- Do not reopen stack-preserved pointer formal post-call overwrite handling
  from idea 359 unless fresh evidence proves this plan is the wrong owner and
  lifecycle state is switched.
- Do not absorb address-valued publication, semantic string loads, frontend
  admission, ABI composite work, variadic/floating work, dynamic-stack work, or
  unrelated scalar compare/select residuals.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00181`, `Hanoi`, one scalar formal name, one source
  statement, one stack offset, one ABI register, or one emitted instruction
  neighborhood.

## Working Model

The historical owner was an AArch64 post-call value-preservation boundary:
generated code computed a later recursive value from clobbered `w0` instead of
reloading the preserved stack home for scalar formal `%p.n`. The parked notes
say this owner was repaired, and the remaining representative failure moved to
the stack-preserved pointer formal path owned by idea 359.

This activation must first prove whether the scalar-formal post-call clobber is
again present. If the focused representative is green or fails for a different
owner, the executor should classify that fact in `todo.md` and return for
lifecycle routing.

## Execution Rules

- Keep routine execution state in `todo.md`.
- Preserve source idea stability; update the source only for durable lifecycle
  decisions such as deactivation, closure, or a true source-intent correction.
- Prefer focused backend evidence before using the external representative as
  acceptance proof.
- Treat expectation rewrites, unsupported downgrades, or named-case shortcuts
  as route failure, not progress.
- Every code-changing step needs fresh build proof plus the supervisor-selected
  focused CTest subset.

## Steps

### Step 1: Refresh Current First Bad Fact

Goal: determine whether the scalar-formal post-call preservation failure is
currently live.

Concrete actions:

- Build the current tree.
- Run the supervisor-selected focused proof for `00181` and nearby existing
  recursive formal preservation guardrails.
- If `00181` passes, report that no current representative first bad fact
  remains for this idea.
- If `00181` fails, inspect current semantic BIR, prepared BIR, and generated
  AArch64 enough to classify the first bad fact.
- Confirm whether scalar formals that remain live after calls are reloaded from
  their preserved homes, or whether generated code again consumes clobbered ABI
  argument registers.

Completion check:

- `todo.md` records the proof command, result, and classification.
- The classification either confirms this idea owns a live scalar-formal
  post-call preservation failure or recommends lifecycle routing because the
  failure is green or belongs to another owner.

### Step 2: Localize the Scalar Formal Boundary

Goal: if Step 1 confirms ownership, identify the exact preservation or reload
boundary where a live scalar formal loses its preserved value after a call.

Primary target:

- AArch64 lowering for post-call uses of scalar formals with prepared preserved
  homes.

Concrete actions:

- Compare semantic BIR, prepared BIR, and generated AArch64 for the smallest
  focused shape that reproduces the clobbered-register consumption.
- Identify the scalar formal, prepared home, intervening call, expected reload,
  actual emitted source register or storage, and first bad consumer.
- Add or update focused backend coverage that fails on the localized boundary
  without relying only on `00181`.

Completion check:

- The owner is described in `todo.md` with concrete prepared and generated-code
  evidence.
- Focused coverage exists or a precise blocker explains why the coverage must
  wait for implementation.

### Step 3: Repair General Post-Call Scalar Formal Use

Goal: make post-call scalar formal consumers reload or publish from the
preserved home instead of using clobbered ABI argument registers.

Concrete actions:

- Implement the narrow AArch64 lowering repair at the localized owner.
- Keep the repair general for scalar formals that remain live across recursive
  or same-module calls.
- Avoid testcase-shaped matching on `00181`, `Hanoi`, `%p.n`, one stack slot,
  one ABI register, or generated instruction text.
- Preserve pointer-formal home publication, stack-preserved pointer formal
  handling, address-valued publication, and unrelated backend behavior.

Completion check:

- Focused backend coverage for the localized scalar-formal shape passes.
- Generated AArch64 shows post-call scalar formal uses reading the preserved
  value rather than a clobbered ABI argument register.
- No expectation, runner, timeout, unsupported, or CTest-registration changes
  are used as proof.

### Step 4: Representative and Guardrail Proof

Goal: prove the repair moves the active representative and keeps adjacent
guardrails stable.

Concrete actions:

- Run the supervisor-selected proof subset, including the focused scalar-formal
  tests and `c_testsuite_aarch64_backend_src_00181_c`.
- Include adjacent recursive pointer-formal, stack-preserved pointer formal,
  address-valued publication, and backend dispatch guardrails if the
  implementation touched shared lowering.
- If `00181` advances to a new first bad fact, classify it without expanding
  this source idea.

Completion check:

- `todo.md` records passing focused proof or the remaining classified failure.
- If the scalar-formal post-call preservation owner is satisfied, return to
  lifecycle routing for closure, parking, or split handling.
