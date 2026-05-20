# AArch64 Unsigned Div Rem Producer Publication Plan

Status: Active
Source Idea: ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md
Activated From: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md

## Purpose

Repair AArch64 scalar producer publication for unsigned division and unsigned
remainder values so later scalar consumers, truncations, comparisons, loop
updates, and selected stores read the actual `udiv`/`urem` result instead of
stale scratch or condition state.

Goal: make unsigned div/rem producers publish their computed result into the
register or storage location consumed by later AArch64 scalar and selected
store operations.

Core Rule: repair the semantic unsigned div/rem producer-publication path; do
not special-case `00182`, the LED digit array, temporary names, registers, or
one emitted instruction neighborhood.

## Read First

- `ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md`
- `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md` for the
  split context and why `00182` is no longer owned by indexed aggregate
  address/writeback.
- Recent scalar publication owners before editing, especially
  `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
  and any closed direct-call, return, or local-conversion publication owners
  referenced by the source idea.

## Current Targets

- AArch64 lowering and printing path from prepared unsigned division/remainder
  scalar operation through emitted producer result publication.
- Consumers of unsigned div/rem results, including truncation, stores, loop
  updates, comparisons, and selected aggregate store values.
- Focused backend coverage that proves unsigned div/rem producers feed later
  scalar consumers before using `00182` as external proof.

## Non-Goals

- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or external test
  contracts.
- Do not fold in recursive call argument preservation for `00176` or `00181`.
- Do not repair dynamic indexed aggregate selected-address/writeback behavior
  already owned by idea 348 unless fresh evidence proves the same unsigned
  div/rem producer boundary is involved.
- Do not expand into signed division/remainder, multiplication, logical shift,
  boolean/comparison materialization, FP expression lowering, semantic
  `lir_to_bir` admission, frame-slot layout, or aggregate call-boundary
  publication without fresh first-bad-fact evidence.

## Working Model

- The split evidence says `00182` reaches selected global stores for the digit
  array, but the value being stored comes from unsigned remainder/truncation,
  and the loop update consumes an unsupported unsigned division producer.
- The suspected failure mode is scalar producer publication: generated AArch64
  leaves consumers reading stale scratch or condition state rather than the
  actual `udiv` or unsigned remainder result.
- `00182` is an external proof target, not an implementation selector.
  Focused backend coverage should identify the repaired producer-publication
  owner first.

## Execution Rules

- Start by localizing whether the first bad fact is unsigned division
  production, unsigned remainder synthesis, result materialization, truncation
  handoff, selected-store handoff, or loop-update consumer handoff.
- Prefer focused backend coverage for unsigned division and remainder values
  feeding ordinary scalar consumers before selected-store coverage.
- Treat a fix that only recognizes `00182`, one temporary, one global array,
  one register, or one exact instruction sequence as route drift.
- Preserve adjacent scalar publication behavior while repairing this owner.
- Escalate to a separate source idea instead of broadening this plan if fresh
  evidence reaches signed div/rem, multiply, shift, comparison, frame layout,
  semantic admission, or recursive call preservation.

## Steps

### Step 1: Localize Unsigned Div Rem Producer Boundary

Goal: identify the first boundary where an unsigned division or unsigned
remainder result stops being the value consumed by later generated code.

Primary target: AArch64 prepared scalar-operation lowering and generated code
for unsigned div/rem producers and their immediate consumers.

Actions:

- Inspect the AArch64 lowering path for unsigned division and unsigned
  remainder from prepared scalar operation through result publication.
- Trace at least one unsigned division consumer and one unsigned remainder
  consumer through generated AArch64.
- Include `00182` only as evidence for the current external symptom; do not use
  its digit buffer shape as the implementation contract.
- Record whether the first bad boundary is producer instruction emission,
  remainder synthesis after `udiv`, result register publication, truncation
  handoff, selected-store value handoff, or loop-update consumer handoff.

Completion check:

- `todo.md` records the concrete first bad boundary, representative generated
  evidence, and the narrow proof subset for the first implementation packet.

### Step 2: Add Focused Unsigned Div Rem Producer Coverage

Goal: prove the semantic producer-publication bug with focused backend coverage
before relying on external c-testsuite proof.

Actions:

- Add or extend focused backend tests for unsigned division and unsigned
  remainder feeding ordinary scalar consumers.
- Add selected-store coverage only after the ordinary scalar consumer boundary
  is clear or if localization proves the selected-store handoff is the first
  bad boundary.
- Keep test names and assertions semantic; do not encode `00182`, digit-array
  details, temporary names, or emitted register names as the contract.

Completion check:

- Focused coverage fails without the repair or existing focused coverage is
  identified that already exposes the unsigned div/rem publication boundary.

### Step 3: Repair Unsigned Div Rem Result Publication

Goal: make AArch64 lowering publish `udiv` and unsigned remainder results into
the scalar value consumed by later operations.

Actions:

- Repair the localized owner from Step 1 in the smallest shared AArch64
  lowering or publication helper that owns unsigned div/rem results.
- Ensure truncations, stores, loop updates, comparisons, and selected-store
  values consume the actual unsigned div/rem result.
- Preserve behavior for adjacent scalar cast, return, direct-call,
  local-conversion, aggregate selected-address, and recursive-call paths.

Completion check:

- Focused backend coverage from Step 2 passes, and the delegated proof subset
  shows no regression in supervisor-selected adjacent scalar publication and
  selected-store guardrails.

### Step 4: Prove External Representative And Reclassify Residuals

Goal: verify that the semantic repair advances `00182` past the stale unsigned
div/rem producer publication failure.

Actions:

- Run the supervisor-selected external proof for `00182` after focused backend
  proof is stable.
- Confirm generated consumers no longer read stale scratch or condition state
  where they should observe unsigned division or remainder results.
- If `00182` remains red, reclassify it by its new first bad fact rather than
  widening this plan by assumption.

Completion check:

- `todo.md` records whether `00182` passed, advanced past the stale
  unsigned-div/rem producer failure, or exposed a new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope unsigned div/rem publication work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent scalar publication and selected-store guardrails remain
  stable.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining unsigned div/rem boundary
  and exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 350 or a clear
  remaining unsigned div/rem producer-publication route that does not broaden
  beyond the source idea.
