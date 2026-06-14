# Plan: Phase F3 Prepared Names Semantic Resolver API Candidate

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Turn one remaining bounded candidate from idea 260 into an executable packet
without claiming broad `PreparedBirModule` field retirement.

## Goal

Implement and prove the `names` semantic resolver API candidate only:
prepared function, block-label, and value-name resolver helpers may expose an
authoritative prepared id or absent-id fact only when lookup never interns,
never falls back to raw BIR ids, and duplicate, empty, absent, and conflicting
spelling rows fail closed.

## Core Rule

Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
field. This plan is one `names` resolver-helper candidate, not a structural
exit for the aggregate.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/names.hpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/lookup_agreement.hpp`
- `src/backend/prealloc/lookup_agreement.cpp`
- Resolver call sites for `resolve_prepared_function_name_id`,
  `resolve_prepared_block_label_id`, `resolve_prepared_value_name_id`, and
  `prepared_named_value_id`
- Focused prepared lookup/helper tests in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- Nearby backend/MIR resolver consumers only as compatibility references, not
  as permission to change target lowering behavior

## Current Scope

- Selected candidate: `names` semantic resolver API packet.
- Primary authority to introduce: resolver helpers may report one
  authoritative prepared id or an explicit absent-id fact for prepared
  function names, block labels, and value names.
- Retained compatibility surface: prepared name tables and existing resolver
  call sites remain authoritative; raw BIR ids, implicit interning, public
  aggregate access, and target/backend fallback behavior must not become
  semantic resolver agreement.

## Non-Goals

- Do not implement another candidate from idea 260 in this runbook.
- Do not change same-block producer lookup, integer-constant lookup,
  value-home lookup, control-flow dominance, branch-target identity,
  block-index label bridging, store-source-publication, printer/debug,
  route-debug, target-output, or backend lowering behavior.
- Do not move target policy, formatting policy, fallback spelling, or
  diagnostics into BIR.
- Do not rewrite helper/oracle status names, unsupported expectations,
  baselines, or output strings to claim progress.
- Do not claim broad structural exit, aggregate retirement, privatization,
  wrapping, or deletion progress.

## Working Model

Prepared name tables remain the owner of public prepared-name behavior. The
semantic resolver API should distinguish an authoritative existing prepared id
from an absent-id fact without mutating tables. Empty spellings, missing names,
duplicate or conflicting rows, invalid raw BIR ids, and prepared/BIR spelling
drift must fail closed. Existing APIs that intentionally intern during
construction, such as `prepared_named_value_id`, must stay separate from
non-interning resolver helpers.

## Execution Rules

- Keep edits local to semantic resolver helper APIs and focused tests.
- Prefer a small resolver result type or helper only if it makes absent,
  present, invalid, and conflicting states explicit without changing existing
  prepared lookup behavior.
- Do not treat raw BIR ids or raw spelling fallback as resolver agreement.
- Preserve current public prepared aggregate compatibility and target/backend
  fallback behavior.
- Prove nearby fail-closed rows, not only one positive testcase.
- Treat expectation rewrites, helper renames, classification-only movement, or
  broad lookup rewrites as non-progress.

## Steps

### Step 1: Inventory Semantic Resolver Contract

Goal: identify the exact current resolver helper behavior, mutation behavior,
and consumer families for the selected candidate.

Actions:

- Inspect `resolve_prepared_function_name_id`,
  `resolve_prepared_block_label_id`, `resolve_prepared_value_name_id`, and
  `prepared_named_value_id`.
- Record which helpers only find existing ids and which helpers intern new
  ids during construction.
- Identify current behavior for empty spelling, absent prepared names,
  invalid raw BIR ids, duplicate or conflicting prepared spelling rows, and
  prepared/BIR spelling drift.
- Identify the narrow test bucket and fixture helpers that can prove semantic
  resolver behavior without pulling in unrelated value-home, same-block,
  control-flow, store-source, or target lowering surfaces.

Completion check:

- `todo.md` names the selected resolver row, current non-interning behavior to
  preserve, candidate implementation files, and a focused proof command for
  the next packet.

### Step 2: Design the Resolver Agreement Boundary

Goal: define when the resolver API can expose an authoritative prepared id or
absent-id fact without changing public prepared-name compatibility.

Actions:

- Choose whether the agreement boundary belongs in `names.hpp`,
  `control_flow.hpp`, `lookup_agreement.*`, or a narrowly scoped adapter
  around resolver query sites.
- Specify accepted rows for function names, block labels, and value names:
  non-empty spelling, existing prepared id, and spelling agreement with the
  prepared name table.
- Specify rejected rows: empty spelling, missing prepared id, invalid raw BIR
  id, duplicate or conflicting spelling rows, prepared/BIR spelling drift, and
  any route that would intern as part of a resolver query.
- Record the design in `todo.md`; do not edit the source idea unless the
  selected candidate itself is ambiguous.

Completion check:

- `todo.md` contains an implementation packet with explicit owned files,
  retained compatibility behavior, and focused fail-closed proof
  requirements.

### Step 3: Implement Narrow Semantic Resolver API

Goal: expose the selected resolver helper boundary while preserving existing
prepared-name ownership.

Actions:

- Implement the narrow resolver result/helper from Step 2.
- Wire it only into selected semantic resolver call sites where it replaces
  ad hoc existing-id checks without changing construction-time interning
  behavior.
- Keep prepared name tables, public aggregate compatibility, raw-spelling
  compatibility, and current null or absent behavior intact.
- Avoid touching same-block producer, value-home, control-flow, store-source,
  printer/debug, route-debug, or backend lowering candidates.

Completion check:

- Build proof passes.
- Focused prepared lookup/helper tests pass.
- The diff does not contain output baseline rewrites, unsupported downgrades,
  construction-time interning changes, or unrelated candidate movement.

### Step 4: Add Fail-Closed Proof Rows

Goal: prove semantic resolver helpers reject ambiguous or stale name facts
while preserving current absent behavior.

Actions:

- Add or preserve positive rows showing agreed prepared function names, block
  labels, and value names resolve to prepared ids.
- Add fail-closed rows for empty spelling, absent prepared names, invalid raw
  BIR ids, duplicate or conflicting spelling, prepared/BIR spelling drift, and
  accidental interning attempts.
- Preserve route-debug, target-output, printer/debug, same-block lookup,
  value-home lookup, control-flow, and store-source expectations unless the
  selected resolver API already owns that surface and the change is explicitly
  proved.

Completion check:

- Focused proof covers positive and nearby fail-closed rows.
- `todo.md` records any unsupported fixture surfaces that should remain out of
  scope rather than being forced into this runbook.

### Step 5: Broader Validation and Closure Readiness

Goal: decide whether the selected candidate is complete and ready for
plan-owner closure review.

Actions:

- Run the focused proof again after any final cleanup.
- Run a broader backend/prepared subset if the resolver helper is shared by
  more than one consumer family.
- Record proof commands, pass/fail result, and residual out-of-scope rows in
  `todo.md`.
- If the selected candidate is complete under idea 260 acceptance criteria,
  request plan-owner closure review; if more candidates remain, close or
  replace this runbook rather than expanding it in place.

Completion check:

- `todo.md` shows the selected candidate complete or blocked with exact
  evidence.
- No other idea 260 candidate has been absorbed into this active plan.
