# Plan: Phase F3 Prepared Names Same-Block Value-Name Lookup Candidate

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Turn one remaining bounded candidate from idea 260 into an executable packet
without claiming broad `PreparedBirModule` field retirement.

## Goal

Implement and prove the `names` same-block value-name lookup candidate only:
same-block producer and integer-constant lookup may use named BIR values only
when unnamed, empty, missing prepared id, stale producer, wrong type,
duplicate spelling, and prepared/BIR name-drift rows fail closed.

## Core Rule

Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
field. This plan is one `names` reader/helper candidate, not a structural exit
for the aggregate.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- Same-block producer and integer-constant lookup tests in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- MIR/BIR query helpers for same-block named producers and integer constants.

## Current Scope

- Selected candidate: `names` same-block value-name lookup packet.
- Primary authority to introduce: named BIR values may support the selected
  same-block producer and integer-constant lookup only under explicit prepared
  value-name agreement.
- Retained compatibility surface: prepared name tables, existing prepared
  lookup maps, and current null or unavailable behavior remain authoritative
  when agreement is missing or ambiguous.

## Non-Goals

- Do not implement another candidate from idea 260 in this runbook.
- Do not move printer/debug strings, route-debug strings, target output,
  fallback spelling, or target policy into BIR.
- Do not change helper/oracle status names, diagnostics, unsupported
  expectations, or baselines to claim progress.
- Do not change value-home lookup, semantic resolver API, control-flow,
  store-source-publication, or backend lowering behavior.
- Do not claim broad structural exit, aggregate retirement, privatization,
  wrapping, or deletion progress.

## Working Model

The prepared lookup path remains the owner of public lookup behavior. A BIR
same-block producer or integer constant can participate only when the named BIR
value is non-empty, maps to an existing prepared `ValueNameId`, agrees with
the prepared row being queried, is in the same block before the cutoff, and is
not made ambiguous by duplicate spelling or prepared/BIR drift. Missing,
stale, unnamed, wrong-type, duplicate, or drift rows must fail closed instead
of falling through to raw spelling as semantic agreement.

## Execution Rules

- Keep edits local to the selected same-block value-name reader path and
  focused tests.
- Prefer one shared agreement helper if both prepared and select-chain lookup
  paths need the same value-name boundary.
- Preserve raw-spelling or prepared-map fallback only as compatibility
  behavior; do not treat it as BIR structural agreement.
- Prove nearby same-feature rows, not only one positive testcase.
- Treat expectation rewrites, helper renames, classification-only movement, or
  broad lookup rewrites as non-progress.

## Steps

### Step 1: Inventory Same-Block Value-Name Contract

Goal: identify the exact current prepared lookup behavior and consumers for
the selected candidate.

Actions:

- Inspect same-block producer and integer-constant lookup code in
  `prepared_lookups.cpp`, `select_chain_lookups.cpp`, and direct query
  helpers.
- Record the current handling of unnamed values, empty names, missing prepared
  ids, stale producers, wrong types, duplicate spelling, and prepared/BIR name
  drift in `todo.md`.
- Identify the narrow test bucket and fixture helpers that already compare
  prepared lookup behavior with MIR/BIR query behavior.

Completion check:

- `todo.md` names the selected lookup row, current behavior to preserve,
  candidate implementation files, and a focused proof command for the next
  packet.

### Step 2: Design the Value-Name Agreement Boundary

Goal: define when named BIR values can be used without changing the prepared
lookup contract.

Actions:

- Choose whether the agreement boundary belongs in an existing lookup helper,
  a small shared prealloc helper, or local adapters in the two lookup files.
- Specify accepted rows: non-empty BIR value name, existing prepared
  `ValueNameId`, same block, before-instruction cutoff, matching producer
  result type, and matching integer-constant interpretation when applicable.
- Specify rejected or compatibility rows: unnamed or empty values, missing
  prepared ids, stale producer references, wrong type, duplicate spelling,
  missing lookup maps, and prepared/BIR name drift.
- Record the design in `todo.md`; do not edit the source idea unless the
  selected candidate itself is ambiguous.

Completion check:

- `todo.md` contains an implementation packet with explicit owned files,
  retained compatibility behavior, and focused fail-closed proof
  requirements.

### Step 3: Implement Narrow Value-Name Bridge

Goal: allow the selected same-block lookup path to use named BIR values only
under the Step 2 agreement boundary.

Actions:

- Implement the narrow agreement helper or adapter from Step 2.
- Wire it only into the selected same-block producer and integer-constant
  lookup paths.
- Keep prepared name tables, existing lookup maps, public aggregate
  compatibility, and current null or unavailable fallback behavior intact.
- Avoid touching unrelated `module`, `names` value-home, semantic resolver,
  `control_flow`, or `store_source_publications` candidates.

Completion check:

- Build proof passes.
- Focused prepared lookup helper tests pass.
- The diff does not contain output baseline rewrites, unsupported downgrades,
  or unrelated candidate movement.

### Step 4: Add Fail-Closed Proof Rows

Goal: prove the selected candidate rejects ambiguous or stale value-name facts
while preserving compatibility behavior.

Actions:

- Add or preserve positive rows showing agreed named BIR values support the
  selected same-block producer and integer-constant lookup.
- Add fail-closed rows for unnamed values, empty names, missing prepared ids,
  stale producer references, wrong result types, duplicate spelling, and
  prepared/BIR name drift.
- Preserve route-debug, target-output, printer/debug, value-home, semantic
  resolver, control-flow, and store-source expectations unless the selected
  lookup path already owns that surface and the change is explicitly proved.

Completion check:

- Focused proof covers positive and nearby fail-closed rows.
- `todo.md` records any unsupported fixture surfaces that should remain out of
  scope rather than being forced into this runbook.

### Step 5: Broader Validation and Closure Readiness

Goal: decide whether the selected candidate is complete and ready for
plan-owner closure review.

Actions:

- Run the focused proof again after any final cleanup.
- Run a broader backend/prepared subset if the lookup helper is shared by more
  than one consumer family.
- Record proof commands, pass/fail result, and residual out-of-scope rows in
  `todo.md`.
- If the selected candidate is complete under idea 260 acceptance criteria,
  request plan-owner closure review; if more candidates remain, close or
  replace this runbook rather than expanding it in place.

Completion check:

- `todo.md` shows the selected candidate complete or blocked with exact
  evidence.
- No other idea 260 candidate has been absorbed into this active plan.
