# Plan: Phase F3 Prepared Module Structural One-Reader Candidate

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Turn one bounded candidate from idea 260 into an executable packet without
claiming broad `PreparedBirModule` field retirement.

## Goal

Implement and prove the `module` lookup-reader candidate only: allow
`prepared_bir_function`, `prepared_bir_block`,
`prepared_bir_block_label_id`, and lookup construction to use a BIR
function/block structure fact when the current prepared lookup is null, while
preserving all existing missing, stale, fallback, and drift behavior.

## Core Rule

Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
field. This plan is one reader/helper-row candidate, not a structural exit for
the aggregate.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- The current definitions and callers for:
  - `prepared_bir_function`
  - `prepared_bir_block`
  - `prepared_bir_block_label_id`
  - prepared lookup construction
- Existing tests around prepared lookup helpers, prepared printer stability,
  backend prepared route/debug rows, and any current null/fallback behavior for
  stale labels or prepared/BIR drift.

## Current Scope

- Selected candidate: `module` lookup-reader packet.
- Primary authority to introduce: one BIR function/block structure agreement
  fact used only when the current prepared lookup would otherwise be null.
- Retained compatibility surface: public prepared aggregate fields and current
  fallback/null behavior remain observable.

## Non-Goals

- Do not implement another candidate from idea 260 in this runbook.
- Do not change printer/debug strings, route-debug strings, target output, or
  baseline expectations to claim progress.
- Do not move target policy, formatting policy, traversal/layout policy, or
  backend output decisions into BIR.
- Do not weaken unsupported behavior or diagnostics.
- Do not claim broad structural exit, aggregate retirement, privatization, or
  wrapper progress.

## Working Model

The existing prepared lookup remains authoritative when it has a valid prepared
answer. The new BIR structure fact may only supply an equivalent answer for the
named lookup-reader surfaces when the prepared path is currently null and the
agreement checks prove the BIR block/function identity matches the prepared
model. Every ambiguous row must keep the existing null, fallback, or
`kInvalidBlockLabel` result.

## Execution Rules

- Keep edits local to the selected lookup-reader helpers and their focused
  tests.
- Add or reuse a semantic helper only if it makes the agreement/fail-closed
  boundary explicit.
- Prove nearby same-feature failure rows, not only one happy path.
- Preserve byte-stable printer/debug/route-debug/target output unless the
  current behavior already changes through the selected helper and is proved.
- Treat expectation rewrites, helper renames, or classification-only movement
  as non-progress.

## Steps

### Step 1: Inventory Lookup-Reader Contract

Goal: identify the exact current behavior and consumers for the selected
candidate.

Actions:

- Inspect the definitions, declarations, and direct callers of
  `prepared_bir_function`, `prepared_bir_block`,
  `prepared_bir_block_label_id`, and prepared lookup construction.
- Record the existing null, fallback, `kInvalidBlockLabel`, stale-label, and
  prepared/BIR drift behavior in `todo.md`.
- Identify the narrow test bucket that already exercises those helpers and the
  nearest fixture surface for adding fail-closed rows.

Completion check:

- `todo.md` names the selected helper row, current behavior to preserve,
  candidate implementation files, and a narrow proof command for the next
  packet.

### Step 2: Design the Agreement Boundary

Goal: define the exact BIR structure fact and compatibility gate before code
  changes.

Actions:

- Choose whether the agreement belongs in an existing prepared lookup helper,
  a small local helper, or a route/query helper already used by the lookup
  construction path.
- Specify the accepted agreement rows: matching function identity, matching
  block identity, valid prepared label id, and no prepared/BIR label drift.
- Specify rejected rows: missing prepared ids, stale BIR labels, raw label
  fallback, duplicate or conflicting labels, invalid ids, and any mismatch that
  currently returns null or `kInvalidBlockLabel`.
- Record the design in `todo.md`; do not edit the source idea unless the
  selected candidate itself is ambiguous.

Completion check:

- `todo.md` contains an implementation packet with explicit owned files,
  retained compatibility behavior, and focused fail-closed proof requirements.

### Step 3: Implement Narrow Lookup-Reader Bridge

Goal: allow the selected lookup-reader helpers to use the BIR structure fact
only under proved agreement.

Actions:

- Implement the narrow agreement helper or adapter from Step 2.
- Wire it into `prepared_bir_function`, `prepared_bir_block`,
  `prepared_bir_block_label_id`, or lookup construction only where the current
  prepared result is null.
- Keep public prepared aggregate compatibility and current fallback/null
  behavior intact.
- Avoid touching unrelated `module`, `names`, `control_flow`, or
  `store_source_publications` candidates.

Completion check:

- Build proof passes.
- Focused helper tests pass.
- The diff does not contain output baseline rewrites, unsupported downgrades,
  or unrelated candidate movement.

### Step 4: Add Fail-Closed Proof Rows

Goal: prove the selected candidate behaves correctly across nearby agreement
and rejection cases.

Actions:

- Add a positive row for the BIR structure fact supplying the same result when
  the prepared lookup would otherwise be null.
- Add rejection rows for missing ids, stale BIR labels, label-string fallback,
  prepared/BIR label drift, invalid labels, and absent control-flow/module
  structure as applicable to the helper under test.
- Preserve existing printer/debug/route-debug and target-output expectations
  unless the selected helper already owns that surface and the change is
  explicitly proved.

Completion check:

- Focused proof covers positive and fail-closed rows.
- `todo.md` records any unsupported fixture surfaces that should remain out of
  scope rather than being forced into this runbook.

### Step 5: Broader Validation and Closure Readiness

Goal: decide whether the selected candidate is complete and ready for
plan-owner closure review.

Actions:

- Run the focused proof again after any final cleanup.
- Run a broader backend/prepared subset if the helper is shared by more than
  one backend or diagnostic surface.
- Record proof commands, pass/fail result, and residual out-of-scope rows in
  `todo.md`.
- If the selected candidate is complete under idea 260 acceptance criteria,
  request plan-owner closure review; if more candidates remain, close or
  replace this runbook rather than expanding it in place.

Completion check:

- `todo.md` shows the selected candidate complete or blocked with exact
  evidence.
- No other idea 260 candidate has been absorbed into this active plan.
