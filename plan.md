# AArch64 Local/Formal Frame-Slot Publication Runbook

Status: Active
Source Idea: ideas/open/353_aarch64_local_formal_frame_slot_publication.md

## Purpose

Reactivate the local/formal frame-slot publication source only long enough to
verify whether its parked first bad fact has returned. Do not assume the
historical `00176` `partition` failure is still current.

## Goal

Repair AArch64 scalar fixed-formal publication into local frame slots only if
fresh generated-code evidence shows local slots are again read before incoming
formal values are published.

## Core Rule

Do not implement from stale parked evidence. Step 1 must refresh the current
first bad fact and either confirm this idea owns a live failure or route the
plan back to lifecycle handling.

## Read First

- `ideas/open/353_aarch64_local_formal_frame_slot_publication.md`
- Prior parked note in that source idea, especially the split to indexed
  aggregate/global array selected-address writeback.
- Generated and prepared artifacts for `00176` only as evidence, not as a
  testcase-specific target.

## Current Targets / Scope

- AArch64 scalar fixed formals that feed local frame slots.
- Publication from incoming ABI registers such as `w0`/`w1` or `x0`/`x1` into
  local stack slots before `load_local` or equivalent consumers read those
  slots.
- Focused backend coverage for formal-to-local initialization and local use
  across at least one call boundary, if the refreshed failure is live.
- Representative proof through `c_testsuite_aarch64_backend_src_00176_c` only
  after the focused owner is confirmed.

## Non-Goals

- Do not reopen block label emission ordering from idea 352.
- Do not reopen recursive call argument preservation, caller-save reloads, or
  preserved-home publication from idea 349.
- Do not absorb indexed aggregate/global array writeback, variadic/byval/HFA,
  scalar-cast stack-home, selected-address, or broad frame-layout work unless
  fresh evidence proves this source idea is the wrong owner and lifecycle state
  is switched.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00176`, `partition`, one local name, one formal
  register, one stack offset, or one emitted instruction neighborhood.

## Working Model

The historical owner was a publication boundary: incoming scalar fixed formals
were available in ABI argument registers on function entry, but generated code
read local frame slots before those slots had been populated. The previous
runbook parked this source after the representative advanced to a distinct
indexed aggregate/global array selected-address writeback residual.

This activation must therefore first prove whether the local/formal
publication boundary is again failing. If the focused representative is green
or fails for a different owner, the executor should classify that fact in
`todo.md` and return for lifecycle routing.

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

Goal: determine whether the local/formal frame-slot publication failure is
currently live.

Concrete actions:

- Build the current tree.
- Run the supervisor-selected focused proof for `00176` and any nearby
  existing local/formal frame-slot publication guardrails.
- If `00176` passes, report that no current representative first bad fact
  remains for this idea.
- If `00176` fails, inspect current prepared BIR and generated AArch64 enough
  to classify the first bad fact.
- Confirm whether the failure still shows incoming scalar formals read from
  stale local frame slots before publication.

Completion check:

- `todo.md` records the proof command, result, and classification.
- The classification either confirms this idea owns a live local/formal
  publication failure or recommends lifecycle routing because the failure is
  green or belongs to another owner.

### Step 2: Localize the Publication Boundary

Goal: if Step 1 confirms ownership, identify the exact lowering boundary where
formal values fail to reach local frame slots.

Primary target:

- AArch64 prepared storage and lowering for scalar fixed formals initialized
  into local slots.

Concrete actions:

- Compare semantic BIR, prepared BIR, selected/prepared AArch64 records, and
  generated assembly for the smallest focused shape that reproduces the
  failure.
- Identify the incoming ABI location, assigned formal home, local frame slot,
  first local-slot consumer, and missing or misplaced publication instruction.
- Add or update focused backend coverage that fails on the localized boundary
  without relying only on `00176`.

Completion check:

- The owner is described in `todo.md` with concrete storage records and
  emitted-code evidence.
- Focused coverage exists or a precise blocker explains why the coverage must
  wait for implementation.

### Step 3: Repair General Formal-to-Local Publication

Goal: publish scalar fixed formals into local frame slots before those local
slots are consumed.

Concrete actions:

- Implement the narrow AArch64 publication repair at the localized owner.
- Keep the repair general for scalar fixed formals feeding local slots.
- Avoid testcase-shaped matching on `00176`, `partition`, local names, register
  numbers, or stack offsets.
- Preserve block-label, recursive call preservation, selected-address, and
  local load/store behavior outside this owner.

Completion check:

- Focused backend coverage for the localized publication shape passes.
- Generated code shows formal values published before local-slot reads.
- No expectation, runner, timeout, unsupported, or CTest-registration changes
  are used as proof.

### Step 4: Representative and Guardrail Proof

Goal: prove the repair moves the active representative and keeps adjacent
guardrails stable.

Concrete actions:

- Run the supervisor-selected proof subset, including the focused publication
  tests and `c_testsuite_aarch64_backend_src_00176_c`.
- Include adjacent AArch64 branch/control, return, call-argument publication,
  selected-address, local load/store, and frame-slot guardrails if the
  implementation touched shared lowering.
- If `00176` advances to a new first bad fact, classify it without expanding
  this source idea.

Completion check:

- `todo.md` records passing focused proof or the remaining classified failure.
- If the local/formal frame-slot owner is satisfied, return to lifecycle
  routing for closure, parking, or split handling.
