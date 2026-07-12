# Prepared-MIR Join-Source Identity Completion Runbook

Status: Active
Source Idea: ideas/open/717_prepared_mir_join_source_identity_completion.md
Resumed after completing: ideas/closed/719_bir_cfg_edge_publication_source_identity_completion.md

## Purpose

Finish acceptance of the repaired common prepared-MIR join-source identity now
that the independently resolved BIR CFG edge-publication prerequisite is
closed.

## Goal

Revalidate complete typed join-source authority across supported and negative
move shapes, prove the route remains general and fail closed, and complete the
broader backend acceptance required before idea 716 can resume.

## Core Rule

Join-source identity comes from exact prepared publication, move, producer,
and freshness facts. Do not infer authority from row order, display names,
route vocabulary, prepared-call facts, or target behavior.

## Read First

- `ideas/open/717_prepared_mir_join_source_identity_completion.md`
- `ideas/closed/719_bir_cfg_edge_publication_source_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/prepared/`
- `src/backend/prealloc/prepared_lookups.cpp`

## Current Scope

- Re-run the focused prepared-MIR direct-edge join-source contract after idea
  719 restored the independent downstream CFG oracle.
- Confirm named, immediate, stack, and unsupported moves preserve their exact
  typed publication, producer, freshness, and move authority.
- Keep the aggregate BIR semantic join view unavailable when required
  publication authority is missing, stale, duplicate, mismatched, or
  unsupported.
- Obtain independent route-quality review and a matching broader backend
  before/after comparison before closure.

## Non-Goals

- Do not change prepared-call plan production or call lookup.
- Do not reopen block-entry or BIR CFG edge-publication identity.
- Do not change target materialization, x86 joined-branch emission, move
  scheduling, register allocation, or diagnostic vocabulary.

## Working Model

- Steps 1 through 3 of the original runbook completed the idea-owned typed
  prepared-MIR join-source repair and focused positive/negative coverage.
- Idea 719 was split out because the next focused failure belonged to an
  independent legacy BIR CFG oracle; that prerequisite is now closed.
- This resumed runbook owns revalidation and closure evidence, not a replay of
  the completed repair unless a focused failure identifies a genuine idea-717
  defect.

## Execution Rules

- Establish the first failing assertion and classify ownership before editing.
- Preserve supported facts even when another move is unsupported, while
  keeping aggregate BIR semantic identity fail closed when required.
- Reject fixture-row, vector-order, display-name, route, prepared-call, and
  target-output shortcuts.
- Do not weaken supported expectations or absorb downstream idea-716 work.
- Use supervisor-delegated build and proof commands and record execution state
  in `todo.md`.

## Ordered Steps

### Step 1: Revalidate the focused join-source boundary

Goal: prove the completed idea-717 repair now runs through its owned assertions
with the idea-719 prerequisite restored.

Actions:

- Inspect the focused prepared-MIR/BIR join-source agreement assertions and
  their supported and fail-closed matrix.
- Run the supervisor-delegated build and focused prepared lookup/join-source
  proof.
- Record the first failing assertion or exception and classify ownership
  before changing code.
- If an idea-717-owned defect remains, repair the general typed join-source
  rule and extend nearby proof without testcase-shaped matching.

Completion check:

- Focused named, immediate, stack, unsupported, missing, stale, duplicate, and
  mismatch assertions pass, or an exact owned semantic defect and bounded
  repair packet are documented in `todo.md`.

### Step 2.1: Preserve exact authority in the prepared-MIR direct-edge view

Goal: ensure an `Available` prepared-MIR source view retains exact typed
authority instead of reducing it to labels, display identities, storage
summaries, and freshness ranks.

Primary targets:

- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/prepared_view.cpp`

Actions:

- Trace the exact prealloc bundle, move, publication, destination/source,
  producer, and selected-freshness-reference facts into the public
  `PreparedMirDirectEdgePublicationSourceView` contract.
- Extend the view with typed identity or an equally exact independently
  resolvable authority path for named, immediate, and stack sources.
- Retain `Available` only when required destination/source, producer,
  publication, move, and selected-freshness authority is complete and
  mutually consistent.
- Classify unsupported, missing, stale, duplicate, or mismatched evidence
  explicitly; do not infer identity from vector position, display names, or
  labels.
- Add prepared-MIR boundary assertions that compare the exact authority to
  the originating publication and move across supported and negative cases.

Completion check:

- Every supported prepared-MIR row exposes exact typed destination/source,
  producer, publication, move, and selected-freshness-reference authority,
  every incomplete row is non-available with its owned classification, and
  the supervisor-delegated build plus narrow prepared-MIR proof is green
  without expectation weakening.

### Step 2.2: Preserve exact authority through the BIR semantic adapter

Goal: prevent the BIR join-source fact from claiming `Available` after exact
prepared-MIR authority has been discarded.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`

Actions:

- Carry or independently resolve the exact prepared-MIR destination/source
  types and identities, producer kind/index, publication, move, and selected
  freshness reference into `BirCurrentBlockJoinSourceFact`.
- Populate existing typed pointer, instruction, and producer fields where
  they are the authoritative representation; add typed fields only where the
  exact contract cannot otherwise be represented.
- Require agreement between the adapted fact and its exact publication,
  move, producer, and freshness evidence before returning `Available`.
- Keep individual rows and the aggregate semantic join identity fail closed
  for unsupported, missing, stale, duplicate, mismatched, or otherwise
  incomplete authority.

Completion check:

- Named, immediate, and stack BIR facts preserve the exact typed authority
  established by Step 2.1, no incomplete fact is `Available`, and the
  supervisor-delegated build plus narrow adapter proof is green.

### Step 2.3: Prove authority preservation across both adapters

Goal: replace status/count-only acceptance with exact semantic proof at each
public boundary.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Assert exact destination/source identity and type, producer kind/index,
  publication identity, move identity, and selected freshness reference after
  the prepared-MIR view and again after BIR adaptation.
- Cover named, immediate, and stack sources plus unsupported, missing, stale,
  duplicate, and mismatched evidence beyond the original four-row fixture.
- Preserve the aggregate fail-closed assertion when any required row lacks
  complete authority.
- Run the supervisor-delegated focused build/test command and preserve its
  output in the canonical executor proof log.

Completion check:

- Focused proof demonstrates exact authority preservation across both
  adapters for supported rows and explicit fail-closed behavior for every
  negative class, without row-shaped shortcuts or expectation downgrades.

### Step 2.4: Re-review route quality and focused acceptance

Goal: independently verify that Steps 2.1 through 2.3 close the blocking
finding in `review/idea717_step2_route_quality_review.md`.

Actions:

- Review the idea-717 implementation from its activation history point through
  `HEAD` against the source idea, this runbook, and the blocking report.
- Reject row-order, display-name, route, expectation, prepared-call, or target
  shortcuts and any per-row or aggregate availability claim lacking exact
  typed authority.
- Confirm the focused assertions prove exact authority rather than only
  status and count.

Completion check:

- Independent review reports no blocking source-alignment, overfit,
  authority-preservation, or proof finding, and focused acceptance is green
  without expectation changes.

### Step 3: Run broader acceptance proof and close

Goal: satisfy the source idea's broader regression requirement and unblock
idea 716.

Actions:

- Run the supervisor-selected matching broader backend before/after comparison
  using canonical regression logs.
- Confirm no new failures and no loss of covered passes.
- Do not begin broader comparison until Step 2.4 accepts the correction.
- Request lifecycle closure only after reviewer and regression acceptance.

Completion check:

- Broader acceptance is green, reviewer reject signals are absent, and idea
  717 can close before resuming idea 716.
