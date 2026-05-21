# AArch64 Static/Global Selected Value Publication Runbook

Status: Active
Source Idea: ideas/open/373_aarch64_static_global_selected_value_publication.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused selected static/global aggregate value publication residual
selected by the backend inventory.

## Goal

Make selected values loaded from static or global aggregates publish to their
scalar consumers, beginning with `00182` digit values passed to LED renderer
calls.

## Core Rule

Repair selected static/global value publication generally. Do not special-case
`00182`, `print_led`, one digit array, one call target, one register, one
stack offset, or one emitted instruction sequence.

## Read First

- `ideas/open/373_aarch64_static_global_selected_value_publication.md`
- `todo.md`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00182.c`
- generated `build/c_testsuite_aarch64_backend/src/00182.c.s`
- semantic/prepared BIR for `00182`
- recent closed `00182`-adjacent owners only as boundary checks

## Current Scope

- selected static local aggregate value loads
- selected global aggregate value loads as adjacency checks
- publication of selected values to scalar call consumers
- representative proof for `c_testsuite_aarch64_backend_src_00182_c`

## Non-Goals

- Do not reopen closed `00182` owners from counts alone.
- Do not broaden into scalar constant-binary stack publication, external call
  result publication, scalar FP materialization, timeout repair, enum
  bit-field layout, or broad aggregate initializer/relocation work.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not implement under umbrella idea 295.

## Working Model

`00182` now advances past older frame-layout, large-immediate, unsigned
div/rem, and selected false-value failure modes. The current generated shape
stores extracted digits into a static local digit array and later uses
selected-load chains for `d[i]`, but the runtime renders all LED digits as
zeroes. Treat the first bad fact as selected static/global value publication
to scalar call consumers until localization proves otherwise.

## Execution Rules

- Start from source, generated AArch64, and semantic/prepared records for
  `print_led` digit extraction and renderer call arguments.
- Trace selected static/global value authority from storage through
  prepared/MIR/AArch64 publication before changing code.
- Add focused coverage before or with the repair.
- Sample `00205` and `00216` only for adjacency after the lead boundary is
  localized.
- Keep parked residual buckets under idea 295 unless a new first bad fact
  proves a lifecycle handoff is required.

## Steps

### Step 1: Localize Selected Value Publication Gap

Goal: identify where selected static/global aggregate values stop reaching
their scalar consumers.

Primary target: generated `00182.c.s`, semantic/prepared BIR for `print_led`,
and AArch64 selected value/call-argument publication helpers.

Actions:

- Inspect the digit extraction stores into the static local digit array.
- Inspect the later selected-load chain for `d[i]` before `topline`,
  `midline`, and `botline`.
- Trace whether selected values are present in semantic/prepared state and
  where they are lost before call-argument publication.
- Compare against recent closed `00182` boundaries so this route does not
  repeat old unsigned div/rem, frame-layout, immediate, or nested-select work.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and focused
  coverage requirement for selected static/global value publication.

### Step 2: Add Focused Selected Value Coverage

Goal: guard selected static/global aggregate value publication independently
of `00182`.

Primary target: backend tests or dump coverage for selected static/global
values feeding scalar consumers.

Actions:

- Add coverage where a selected static or global aggregate element feeds a
  scalar call argument or equivalent consumer.
- Assert the selected value reaches the consumer, not a stale or zero fallback.
- Keep coverage semantic and not tied to one digit array, one call target, one
  register, or one stack offset.

Completion check:

- Focused coverage fails before the repair or directly guards the selected
  value publication fact.

### Step 3: Repair Selected Static/Global Value Publication

Goal: publish selected static/global aggregate values to scalar consumers.

Primary target: the semantic/prepared/MIR/AArch64 helper localized in Step 1.

Actions:

- Implement the smallest semantic repair at the owning publication boundary.
- Preserve recent selected-address, pointer-local, scalar publication, and
  aggregate snapshot repairs.
- Avoid broad rewrites of unrelated scalar constant, external-call, FP,
  timeout, bit-field, or initializer owners.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 no longer collapses selected
  digit values to zero before renderer calls.

### Step 4: Prove Representative And Classify Adjacency

Goal: prove `00182` advances and classify whether adjacent cases share the
same boundary.

Primary target: focused backend coverage,
`c_testsuite_aarch64_backend_src_00182_c`, and adjacency reads for `00205` /
`00216`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00182.c.s` enough to confirm selected values feed the
  renderer calls.
- Sample `00205` and `00216` only enough to classify whether their first bad
  facts share this owner or remain parked.
- If `00182` still fails, classify the new first bad fact and decide whether
  it remains in this idea or needs lifecycle handoff.
- Ask the supervisor whether backend-regex or broader regression guard proof
  is needed before closure.

Completion check:

- `00182` no longer fails because selected digit values collapse to zero
  before scalar consumers, and proof/classification is recorded in `todo.md`.
