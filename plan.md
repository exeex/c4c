# AArch64 Unsigned Div/Rem Producer Publication Runbook

Status: Active
Source Idea: ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md

## Purpose

Refresh and, only if still current, repair the AArch64 unsigned division and
unsigned remainder producer-publication owner recorded in the source idea.

## Goal

Make generated AArch64 consumers observe the actual `udiv` / unsigned
remainder result instead of stale scratch, condition, or unrelated carrier
state.

## Core Rule

Do not assume the historical `00182` first bad fact still exists. Start by
proving the current first bad fact before changing code.

## Read First

- `ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md`
- The current generated AArch64 and prepared artifacts for the selected
  representative, especially `00182` if it is still the leading witness.
- Existing focused backend coverage for unsigned division and unsigned
  remainder producer publication.

## Current Scope

- AArch64 scalar producer publication for unsigned division and unsigned
  remainder.
- Consumers fed by those producers, including truncation, stores, loop
  updates, comparisons, and selected store values.
- Focused backend contracts that prove the producer-publication rule
  independent of a single external c-testsuite file.

## Non-Goals

- Do not reopen dynamic indexed aggregate selected-address/writeback repairs
  from the split parent unless fresh first-bad-fact evidence proves this same
  owner.
- Do not broaden into recursive call preservation, signed div/rem lowering,
  multiplication, logical shift, semantic admission, frame-slot layout, or
  aggregate call-boundary work.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not special-case `00182`, the LED digit array, one temporary, one
  register, or one emitted instruction sequence.

## Working Model

The historical failure was that generated AArch64 consumers could read stale
condition or scratch state where the unsigned division or unsigned remainder
producer should have been published. The old parking note says the repair had
landed and `00182` advanced to a frame-size segfault, so the current run must
first confirm whether this source idea still owns any live failure.

## Execution Rules

- Prefer generated-code, prepared-BIR, or focused dump evidence over external
  output alone.
- If the current first bad fact is not unsigned div/rem producer publication,
  classify it in `todo.md` and return for lifecycle routing rather than
  widening this plan.
- Any repair must add or preserve focused backend coverage that fails without
  the general producer-publication behavior.
- Keep proof narrow at first, then escalate only if the code slice touches
  broader scalar publication behavior.

## Step 1: Refresh Current First Bad Fact

Goal: determine whether unsigned div/rem producer publication is still a live
owner.

Primary target: the supervisor-selected focused representative for this idea,
normally `c_testsuite_aarch64_backend_src_00182_c` plus existing focused
unsigned div/rem backend contracts.

Actions:

- Rebuild the current tree before classification.
- Run the focused representative and nearby unsigned div/rem backend coverage.
- Inspect current prepared/generated artifacts for the first bad fact.
- Decide whether any observed failure still shows consumers reading stale
  state instead of an unsigned division or unsigned remainder result.

Completion check:

- `todo.md` records either a current unsigned div/rem producer-publication bad
  fact with concrete evidence, or a current out-of-scope / already-green
  classification for supervisor lifecycle routing.

## Step 2: Localize The Producer Handoff

Goal: if Step 1 finds an in-scope failure, identify the exact boundary where
the produced unsigned value stops being available to its consumer.

Primary target: AArch64 scalar operation selection, result materialization,
and consumer handoff for `udiv` and unsigned remainder.

Actions:

- Compare semantic BIR, prepared BIR, selected records, and emitted assembly
  for the failing producer and first stale consumer.
- Identify whether the gap is in producer result storage, prepared move
  creation, selected-node normalization, register publication, or a later
  consumer load.
- Record the smallest focused backend shape that should fail without the
  repair.

Completion check:

- The failing boundary is named in terms of producer, carrier, expected
  storage, observed stale storage, and first consumer.

## Step 3: Repair General Publication

Goal: repair the localized unsigned div/rem producer-publication rule without
testcase-shaped matching.

Actions:

- Implement the smallest general lowering or publication change needed for the
  localized owner.
- Add or update focused backend coverage for unsigned division and/or unsigned
  remainder feeding later scalar consumers.
- Keep adjacent selected-store and scalar publication behavior stable.

Completion check:

- Focused backend coverage proves consumers read the actual unsigned div/rem
  result.
- The implementation does not mention `00182`, one digit array, one temporary
  id, or one emitted instruction sequence as a control path.

## Step 4: Prove And Reclassify

Goal: prove the repaired owner and hand off any remaining first bad fact.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include `00182` representative proof if it remains the external witness.
- If the representative still fails, classify the new first bad fact and keep
  it separate from this unsigned div/rem source scope.

Completion check:

- `todo.md` records the proof command and result.
- The source idea is either ready for close consideration, or the next first
  bad fact is clearly classified for lifecycle handoff.
