# AArch64 Recursive Call Argument Preservation Plan

Status: Active
Source Idea: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Activated After: ideas/closed/351_aarch64_nested_select_false_value_materialization.md
Activated From: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md

## Purpose

Repair AArch64 generated-code handling for live values that must survive a
nested or recursive `bl` before later control-flow, array mutation, or calls
consume the original source value.

Goal: make post-call uses read preserved or reloaded source values instead of
stale caller-clobbered argument registers.

Core Rule: repair general live-value preservation across caller-clobbering
calls; do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one
argument index, one register name, or one emitted call neighborhood.

## Read First

- `ideas/open/349_aarch64_recursive_call_argument_preservation.md`
- `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md` for the
  split boundary: indexed aggregate address/writeback was parked after
  representatives advanced to recursive call argument preservation.
- AArch64 prepared call lowering, live-value storage assignment, caller-save
  handling, post-call reload/materialization, and selected argument
  publication paths before editing.

## Current Targets

- Current representatives: `c_testsuite_aarch64_backend_src_00176_c` and
  `c_testsuite_aarch64_backend_src_00181_c`.
- First bad fact family: generated recursive or nested call sequences reuse
  caller-clobbered argument registers such as `w0` and `w1` after a `bl` when
  the original low/high or Tower-of-Hanoi arguments are still live.
- Focused backend coverage for a recursive or nested ordinary call where a
  caller-side value is used after the nested call returns.
- Adjacent call publication and aggregate selected-address guardrails chosen
  by the supervisor.

## Non-Goals

- Do not reopen dynamic indexed aggregate selected-address/writeback repairs
  owned by idea 348 unless fresh generated-code evidence reaches that exact
  boundary again.
- Do not broaden into variadic aggregate `va_arg`, byval aggregate lane
  publication, HFA/floating aggregate publication, fixed-formal entry
  publication, scalar cast publication, return publication, local conversion
  publication, frame-slot layout, runner behavior, timeout policy,
  expectation changes, unsupported-classification changes, CTest
  registration, or proof-log behavior.
- Do not use filename-only, function-name-only, register-name-only,
  diagnostic-string-only, c-testsuite-number-specific, or emitted-text-only
  fixes.

## Working Model

- The source value remains live across the nested call in the BIR/prepared
  model, but generated AArch64 later consumes the original ABI argument
  register after the call.
- A nested or recursive `bl` may clobber caller argument registers, so any
  later use must come from a preserved home, caller-save spill, reload, or
  rematerialized value under the prepared storage contract.
- The repair should make post-call value consumption follow the prepared
  live-value or local-storage handoff instead of assuming incoming argument
  registers remain valid.

## Execution Rules

- Start from generated/prepared evidence for the first bad boundary before
  modifying code.
- Prefer a shared live-value preservation, caller-save, reload, or selected
  operand-publication repair over emitted-instruction pattern matching.
- Preserve existing call argument publication, return-value publication,
  selected aggregate address/writeback, scalar publication, and frame-layout
  guardrails.
- Treat any fix that recognizes only `00176`, `00181`, `quicksort`, `Hanoi`,
  one argument slot, one register pair, or one call sequence as route drift.
- Escalate to a separate source idea if fresh evidence reaches indexed
  aggregate writeback, variadic/byval publication, frame layout, semantic
  admission, or unrelated runtime mismatch work.

## Steps

### Step 1: Localize Recursive Call Preservation Boundary

Goal: identify the exact AArch64 boundary where a live caller-side value that
must survive a nested or recursive `bl` remains tied only to a caller-clobbered
argument register.

Primary target: prepared live-value records, assigned homes, call lowering,
caller-save handling, post-call reload/materialization, and selected
post-call operand publication.

Actions:

- Trace the prepared and selected records for at least one current
  representative, starting with `00176` or `00181` as the supervisor chooses.
- Identify the source value, its prepared storage, the nested `bl` that may
  clobber the argument register, and the later generated use that reads stale
  caller-clobbered state.
- Decide whether the owner is missing caller-save spill, missing reload from a
  prepared home, stale selected operand binding, local publication, or another
  call-boundary handoff with direct evidence.

Completion check:

- `todo.md` records the concrete first bad boundary, representative prepared
  and generated evidence, and the narrow proof subset for the first
  implementation packet.

### Step 2: Add Focused Recursive/Nested Call Coverage

Goal: prove live caller-side values survive a nested or recursive call without
depending only on external c-testsuite output.

Actions:

- Add or extend focused backend coverage for an ordinary nested or recursive
  call where a value passed as an argument is used again after the nested call.
- Assert that post-call generated code reloads, preserves, or rematerializes
  the original source value before the later use.
- Keep test contracts independent of `00176`, `00181`, `quicksort`, `Hanoi`,
  one register name, or one emitted instruction neighborhood.

Completion check:

- Focused coverage fails without the repair or an existing focused test is
  identified that already exposes the caller-clobbered post-call reuse.

### Step 3: Repair Post-Call Value Consumption

Goal: make generated AArch64 post-call uses consume preserved or reloaded live
values instead of stale caller-clobbered argument registers.

Actions:

- Repair the localized owner from Step 1 in the smallest shared prepared,
  selected, caller-save, reload, or operand-publication helper.
- Ensure post-call control-flow, array mutation, and later call setup consume
  the source value from a valid post-call location.
- Preserve existing direct-call publication, return publication, aggregate
  selected-address, scalar publication, and frame-layout behavior.

Completion check:

- Focused coverage from Step 2 passes, and supervisor-selected adjacent
  call/publication guardrails show no regression.

### Step 4: Prove Representatives And Reclassify Residuals

Goal: verify the current external representatives advance past the
caller-clobbered recursive argument reuse failure and classify any new first
bad fact.

Actions:

- Run the supervisor-selected external proof for `00176` and `00181` after
  focused proof is stable.
- Confirm at least one representative advances past the stale post-call
  argument-register reuse failure.
- If either representative remains red, reclassify the new first bad fact
  instead of widening this plan by assumption.

Completion check:

- `todo.md` records whether `00176` and `00181` passed, advanced past the
  recursive call preservation failure, or exposed a new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope call-preservation work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent call publication and aggregate selected-address guardrails
  remain stable.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining call-preservation boundary
  and exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 349 or a clear
  remaining recursive-call preservation route that does not broaden beyond the
  source idea.
