# LIR Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/857_lir_next_body_parameter_authority_handoff.md
Activated From: ideas/open/734_lir_to_new_bir_container_completeness.md post-Step 7.45 close rejection

## Purpose

Select and prove one producer-side LIR authority handoff for the next valid
function-body parameter-use row after 734 Step 7.45.

## Goal

Publish exactly one structured body-parameter authority tuple that can later be
received by 734 into typed Raw BIR.

## Core Rule

Producer authority must be native structured LIR data. Do not derive identity,
type, role, or consumer coherence from text, names, rendered operands,
signatures, compatibility mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/857_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- Accepted 734 Step 7.45 receiver commit `2b7e897ce`
- Closed 856 handoff close commit `94ffc9d30`
- Existing LIR body-parameter authority producer/verifier patterns for the
  accepted 817, 818/820, 823, 824, 825, 826, 827, 854, 855, and 856 handoffs

## Current Targets And Scope

- Trace only the next valid function-body parameter-use row after the accepted
  DirectPointer and DirectScalar body-parameter receipts through 734 Step 7.45.
- Publish only the selected row's structured parameter source identity,
  current-function owner, parameter index, type, native body-parameter ABI,
  explicit role, and consumer relation coherence.
- Add focused verifier and malformed-authority coverage for the selected row.
- Produce a one-row handoff back to 734 after the producer/schema/verifier
  proof accepts.

## Non-Goals

- Do not edit Raw-BIR containers, builders, importers, verifier receipt, or
  receiver tests.
- Do not repeat accepted 734 body-parameter receiver rows through Step 7.45.
- Do not select from presentation fields or compatibility fallbacks.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or more than one
  body-parameter row.
- Do not weaken unsupported diagnostics, expectation contracts, or fail-closed
  malformed-authority behavior.

## Execution Rules

- Keep changes producer/schema/verifier-side until the handoff is closed.
- If tracing finds no receiver-ready body-parameter row with native structured
  authority, record the blocker instead of inventing authority.
- Coverage must include neighboring malformed cases appropriate to the
  selected row: omitted or missing authority, invalid value, duplicate
  definition, foreign owner, wrong index, wrong type, wrong ABI, wrong role,
  and selected-consumer incoherence.
- For code changes, run a fresh build, focused same-feature proof, and
  `git diff --check`. Escalate to a broader LIR/frontend guard if shared
  verifier or producer code is touched.

## Steps

### Step 1 - Trace and select the next body-parameter authority row

Goal: identify exactly one valid function-body parameter-use row after 734
Step 7.45 that needs producer authority before Raw-BIR receipt.

Actions:

- Inspect the accepted body-parameter authority chain and current LIR
  body-parameter verifier failures.
- Select one native producer/consumer row only if it can carry structured
  parameter identity, owner, index, type, ABI, role, and consumer coherence.
- Record nonselected families as fail-closed or separately blocked; do not
  collapse them into this route.

Completion check:

- The selected row, producer surface, consumer relation, and malformed matrix
  are explicit enough for one bounded repair step.

### Step 2 - Publish and verify the selected authority tuple

Goal: add only the LIR producer/schema/verifier authority required for the
selected one-row handoff.

Actions:

- Add native structured carrier fields or producer binding only where the
  selected row requires them.
- Extend verifier checks so malformed authority rejects before downstream use.
- Add nearby positive and malformed-authority coverage for the selected row.
- Preserve fail-closed behavior for every nonselected row.

Completion check:

- Fresh build passes.
- Focused same-feature producer/verifier proof passes.
- `git diff --check` passes.
- No Raw-BIR/importer/receiver code changed.

### Step 3 - Record the handoff back to 734

Goal: make the accepted producer tuple executable as a later 734 receiver
packet.

Actions:

- Update lifecycle state with the exact selected tuple, accepted proof, and
  receiver exclusions.
- Return control to 734 only after the producer/schema/verifier route is
  accepted by the supervisor.

Completion check:

- 857 names one exact handoff row for 734, and all nonselected rows remain
  explicitly outside the handoff.
