# AArch64 Nested Select False Value Materialization Plan

Status: Active
Source Idea: ideas/open/351_aarch64_nested_select_false_value_materialization.md
Activated From: ideas/closed/316_aarch64_frame_slot_layout_consistency.md

## Purpose

Repair AArch64 nested scalar select/value materialization when a later
condition check must not destroy the accumulated false operand selected by
earlier arms.

Goal: make generated AArch64 preserve or rematerialize the accumulated false
select value so final `csel` consumers observe the same value modeled by
prepared BIR.

Core Rule: repair the scalar select/value materialization contract; do not
special-case `00182`, `print_led`, LED line renderers, the digit array,
`d[0]`, one register, or one emitted instruction neighborhood.

## Read First

- `ideas/open/351_aarch64_nested_select_false_value_materialization.md`
- `ideas/closed/316_aarch64_frame_slot_layout_consistency.md` for why the
  frame-layout owner is complete and why the new `00182` residual is separate.
- The AArch64 scalar select lowering, value materialization, operand
  publication, and condition-register/scratch-register handling paths before
  editing.

## Current Targets

- Current `00182.c` residual: `print_led` computes digits correctly in
  prepared BIR, but generated AArch64 collapses the final digit lookup select
  so both final `csel` operands hold `d[0]`.
- Focused backend coverage for a nested select chain where the final condition
  is false and the expected result comes from the accumulated false operand.
- Adjacent scalar select publication and scalar operand materialization
  guardrails to ensure the repair does not regress previous select work.

## Non-Goals

- Do not reopen idea 316's frame-slot/frame-size layout owner.
- Do not reopen unsigned div/rem producer publication, digit extraction
  semantics, LED formatting, direct-call lowering, local-array address
  formation, prepared call/address materialization, variadic lowering, runner
  behavior, timeout policy, expectation changes, unsupported-classification
  changes, or CTest registration.
- Do not use filename-only, function-name-only, literal-register-only,
  diagnostic-string-only, c-testsuite-number-specific, or emitted-text-only
  fixes.

## Working Model

- The old local-array frame failure is repaired: `00182` now reaches stable LED
  rendering instead of corrupting `main`'s saved return address.
- The current first bad fact is nested scalar select value materialization. A
  final condition comparison overwrites or aliases the value that should
  represent the previously accumulated false select result.
- The repair should make the final select choose between the current true
  value and the accumulated false value, not between two copies of the current
  true value.

## Execution Rules

- Start from focused select/value materialization evidence before relying on
  external `00182` output.
- Prefer a shared scalar select lowering or operand materialization repair over
  emitted-instruction pattern matching.
- Preserve condition evaluation, scratch-register lifetime, stack-home
  publication, and previous scalar select publication behavior.
- Treat any fix that recognizes only `00182`, `print_led`, LED line renderers,
  `d[0]`, one temporary, one register, or one instruction sequence as route
  drift.
- Escalate to a separate source idea if fresh evidence reaches frame layout,
  unsigned div/rem, call lowering, semantic admission, aggregate storage, or
  unrelated runtime mismatch work.

## Steps

### Step 1: Localize Nested Select Value Boundary

Goal: identify the exact AArch64 boundary where the accumulated false operand
for a nested select chain is lost, overwritten, or not materialized for the
final `csel`.

Primary target: scalar select lowering, operand materialization, scratch
register ownership, and condition evaluation around nested select chains.

Actions:

- Trace the prepared BIR select chain for the current `00182.c` digit lookup
  and compare it with generated AArch64 operand lifetimes.
- Identify whether the first bad boundary is select lowering, false-operand
  materialization, scratch register reuse, condition-code clobber handling, or
  stack-home publication.
- Check whether idea 345's closed stack-home publication failure has actually
  returned or whether this is a distinct register/value preservation problem.

Completion check:

- `todo.md` records the concrete first bad boundary, representative prepared
  and generated evidence, and the narrow proof subset for the first
  implementation packet.

### Step 2: Add Focused Nested Select Coverage

Goal: prove nested select false-value materialization without depending on
`00182` output formatting.

Actions:

- Add or extend focused backend coverage for nested scalar selects where the
  final condition is false.
- Assert that the selected value comes from the accumulated false operand,
  including a case where the first true candidate differs from the false
  result.
- Keep test contracts independent of `00182`, LED rendering, one register, or
  one emitted instruction string.

Completion check:

- Focused coverage fails without the repair or an existing focused test is
  identified that already exposes the nested select value loss.

### Step 3: Repair Select Operand Preservation

Goal: make generated AArch64 preserve or rematerialize the accumulated false
select value across later condition checks.

Actions:

- Repair the localized owner from Step 1 in the smallest shared scalar select
  lowering or operand materialization helper.
- Ensure final `csel` operands name the current true value and the accumulated
  false value when the select model requires that distinction.
- Preserve existing scalar select result publication, scalar cast/source
  materialization, and condition evaluation behavior.

Completion check:

- Focused coverage from Step 2 passes, and supervisor-selected adjacent scalar
  select/materialization guardrails show no regression.

### Step 4: Prove External Representative And Reclassify Residuals

Goal: verify that `00182` advances past the seven-`7` select/materialization
failure and classify any remaining first bad fact.

Actions:

- Run the supervisor-selected external proof for `00182` after focused proof
  is stable.
- Confirm generated AArch64 no longer passes `d[0]` for every false-path digit
  lookup because the accumulated false select value was lost.
- If `00182` remains red, reclassify it by the new first bad fact rather than
  widening this plan by assumption.

Completion check:

- `todo.md` records whether `00182` passed, advanced past the nested select
  false-value failure, or exposed a new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope select/materialization work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent scalar select publication, operand materialization, and
  frame-layout guardrails remain stable.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining select/materialization
  boundary and exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 351 or a clear
  remaining select/value materialization route that does not broaden beyond
  the source idea.
