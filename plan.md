# AArch64 Unsigned Div Rem Producer Publication Refresh Runbook

Status: Active
Source Idea: ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md

## Purpose

Refresh the parked unsigned division/remainder producer-publication owner and
decide whether any current first bad fact still belongs to the source idea.

## Goal

Prove whether unsigned `udiv` or `urem` results are still read from stale
scratch or condition state by later AArch64 scalar consumers, or classify the
current tree as out of scope for this idea.

## Core Rule

Do not reopen selected aggregate address/writeback, recursive call argument
preservation, signed arithmetic, frame-slot layout, or unrelated scalar ALU
publication unless fresh generated-code evidence makes that boundary the
current first bad fact.

## Read First

- `ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md`
- Prior split source idea for context:
  `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md` if present
- `test_before.log` and `test_after.log` if present, because the source note
  records prior close-gate rejection on matching narrow logs.

## Current Scope

- Focused backend coverage for unsigned division and remainder scalar
  producers.
- `c_testsuite_aarch64_backend_src_00182_c` as the representative external
  proof only after focused producer coverage is checked.
- Adjacent scalar publication and selected-store guardrails selected by the
  supervisor.
- A fresh proof record suitable for the supervisor and close-gate policy.

## Non-Goals

- Do not change implementation files during lifecycle activation.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00182`, the LED digit array, one temporary, one
  register, one condition flag, or one emitted instruction sequence.
- Do not expand this runbook into selected aggregate writeback, recursive call
  preservation, signed div/rem, multiplication, logical shift, semantic
  admission, or frame-layout work without a lifecycle split.

## Working Model

Idea 350 originally owned AArch64 scalar producer publication where unsigned
division or remainder results were computed but later consumers read stale
scratch or condition state. Prior execution reports the focused unsigned
div/rem subset and `00182` green, while closure was rejected only because the
strict close-time regression guard did not see a pass-count increase. This
runbook refreshes that conclusion from the current tree.

## Execution Rules

- Start from focused backend tests and generated-code evidence, not from the
  historical failure alone.
- If the focused unsigned div/rem producer subset and `00182` are still green,
  do not claim new in-scope repair work under this source idea.
- If `00182` fails later, classify the first bad fact before broadening this
  route.
- Preserve canonical proof-log names when a delegated executor writes logs:
  `test_before.log` and `test_after.log`.
- Keep routine proof details in `todo.md`; edit this plan only if the refresh
  route itself needs correction.

## Step 1: Refresh Current Unsigned Div/Rem Producer Proof

Goal: determine whether the current tree still exposes an unsigned division or
remainder producer-publication failure owned by this source idea.

Primary target: focused unsigned div/rem backend coverage and
`c_testsuite_aarch64_backend_src_00182_c`

Actions:

- Rebuild the default preset.
- Run the supervisor-selected focused proof for unsigned div/rem producer
  publication and `00182`.
- If any test fails, capture the earliest observable failure and compare it to
  the source idea's stale unsigned div/rem producer boundary.
- If needed, inspect prepared BIR and generated AArch64 only enough to
  classify the first bad fact.

Completion check:

- `todo.md` records the exact proof command, result, and whether the first bad
  fact is in scope for idea 350.

## Step 2: Preserve Adjacent Boundaries

Goal: ensure the refresh result does not blur nearby owners that idea 350
explicitly excludes.

Primary target: adjacent scalar publication and selected-store guardrails
chosen by the supervisor.

Actions:

- Include guardrails for scalar ALU records, prepared scalar ALU records,
  instruction dispatch, and any selected-store consumer checks the supervisor
  requires.
- Treat selected aggregate address/writeback, recursive call preservation,
  signed arithmetic, frame-slot layout, or unrelated scalar publication
  failures as out-of-scope handoffs unless fresh evidence proves they are
  required to explain unsigned div/rem consumers.

Completion check:

- `todo.md` records whether adjacent guardrails passed and names any
  out-of-scope residual that should be handled by another source idea.

## Step 3: Lifecycle Decision Handoff

Goal: leave the supervisor with a clear close, deactivate, or split decision.

Actions:

- If no in-scope unsigned div/rem producer failure remains, recommend
  deactivation or closure attempt under the close gate.
- If an in-scope producer-publication failure remains, hand the localized owner
  to an executor packet.
- If the first bad fact belongs elsewhere, recommend the matching existing
  source idea or a new split.

Completion check:

- `todo.md` contains a concise latest-packet summary, proof result, and
  suggested next lifecycle action.
