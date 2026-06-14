# Plan: Phase F3 Prepared Names Value-Home Lookup Candidate

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Turn one remaining bounded candidate from idea 260 into an executable packet
without claiming broad `PreparedBirModule` field retirement.

## Goal

Implement and prove the `names` value-home lookup candidate only: value-home
lookup may use one authoritative prepared `ValueNameId` or absent-id fact only
when null function locations, immediate values, empty names, missing prepared
names, missing homes, and stale indexed lookups preserve current null behavior.

## Core Rule

Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
field. This plan is one `names` reader/helper candidate, not a structural exit
for the aggregate.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/lookup_agreement.hpp`
- `src/backend/prealloc/lookup_agreement.cpp`
- Value-home lookup and prepared lookup helper tests in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- Nearby value-home consumers in `src/backend/prealloc/variadic_entry_plans.cpp`
  and publication/decoded-home tests only as compatibility references.

## Current Scope

- Selected candidate: `names` value-home lookup packet.
- Primary authority to introduce: a named BIR value may resolve to a prepared
  value home only through one authoritative prepared `ValueNameId` or an
  explicit absent-id fact.
- Retained compatibility surface: prepared name tables, prepared value-home
  records, prepared value-home lookup indexes, and current null behavior remain
  authoritative when agreement is missing, stale, ambiguous, or unsupported.

## Non-Goals

- Do not implement another candidate from idea 260 in this runbook.
- Do not change same-block producer lookup, integer-constant lookup, semantic
  resolver API, control-flow, store-source-publication, printer/debug,
  route-debug, target-output, or backend lowering behavior.
- Do not move target policy, formatting policy, fallback spelling, or
  diagnostics into BIR.
- Do not rewrite helper/oracle status names, unsupported expectations,
  baselines, or output strings to claim progress.
- Do not claim broad structural exit, aggregate retirement, privatization,
  wrapping, or deletion progress.

## Working Model

Prepared value-home lookup remains the owner of public value-home behavior. A
named BIR value can participate only when the spelling is non-empty, the
prepared name table has exactly one matching `ValueNameId`, the requested
function locations exist, the prepared value-home record exists for that id,
and any supplied index agrees with that prepared record. Null function
locations, immediate values, empty or absent names, missing prepared ids,
missing homes, stale indexes, and drift between a cached index and prepared
locations must return the same null result as the current prepared lookup
contract.

## Execution Rules

- Keep edits local to the selected value-home lookup path and focused tests.
- Prefer a small shared agreement helper only if it removes duplication with
  existing name agreement helpers and stays scoped to prepared lookup
  boundaries.
- Preserve raw name or index compatibility only as current prepared behavior;
  do not treat it as BIR structural agreement.
- Prove nearby fail-closed rows, not only one positive testcase.
- Treat expectation rewrites, helper renames, classification-only movement, or
  broad lookup rewrites as non-progress.

## Steps

### Step 1: Inventory Value-Home Lookup Contract

Goal: identify the exact current value-home lookup behavior and consumers for
the selected candidate.

Actions:

- Inspect value-home lookup helpers in `value_locations.hpp` and
  `prepared_lookups.cpp`, including `make_prepared_value_home_lookups`,
  `find_prepared_value_home`, `find_indexed_prepared_value_home`, and
  `find_prepared_value_home_for_bir_value`.
- Record current handling of null function locations, immediate values, empty
  names, missing prepared names, missing homes, stale indexed lookups, and
  duplicate or conflicting prepared names in `todo.md`.
- Identify the narrow test bucket and fixture helpers that can prove
  value-home lookup behavior without pulling in unrelated publication,
  decoded-home, semantic resolver, or backend lowering surfaces.

Completion check:

- `todo.md` names the selected lookup row, current null behavior to preserve,
  candidate implementation files, and a focused proof command for the next
  packet.

### Step 2: Design the Value-Home Agreement Boundary

Goal: define when a named BIR value can use prepared value-home authority
without changing the public lookup contract.

Actions:

- Choose whether the agreement boundary belongs in `lookup_agreement.*`,
  `value_locations.hpp`, `prepared_lookups.cpp`, or a local adapter around the
  value-home query sites.
- Specify accepted rows: named non-empty BIR value, existing prepared
  `ValueNameId`, available function locations, existing prepared value-home
  record, and matching indexed lookup record when an index is supplied.
- Specify rejected rows: null function locations, immediate values, empty
  names, missing prepared ids, missing homes, stale indexes, duplicate or
  conflicting names, and prepared/BIR name drift.
- Record the design in `todo.md`; do not edit the source idea unless the
  selected candidate itself is ambiguous.

Completion check:

- `todo.md` contains an implementation packet with explicit owned files,
  retained compatibility behavior, and focused fail-closed proof
  requirements.

### Step 3: Implement Narrow Value-Home Bridge

Goal: allow the selected value-home lookup path to use named BIR values only
under the Step 2 agreement boundary.

Actions:

- Implement the narrow agreement helper or adapter from Step 2.
- Wire it only into the selected value-home lookup path.
- Keep prepared name tables, prepared value-home records, prepared lookup
  indexes, public aggregate compatibility, and current null fallback behavior
  intact.
- Avoid touching same-block producer, integer-constant, semantic resolver,
  `control_flow`, `store_source_publications`, printer/debug, route-debug, or
  backend lowering candidates.

Completion check:

- Build proof passes.
- Focused prepared lookup helper tests pass.
- The diff does not contain output baseline rewrites, unsupported downgrades,
  or unrelated candidate movement.

### Step 4: Add Fail-Closed Proof Rows

Goal: prove the selected candidate rejects ambiguous or stale value-home facts
while preserving current null behavior.

Actions:

- Add or preserve positive rows showing agreed named BIR values resolve to the
  prepared value-home authority.
- Add fail-closed rows for null function locations, immediate values, empty
  names, missing prepared ids, missing homes, stale indexed lookups, duplicate
  or conflicting names, and prepared/BIR name drift.
- Preserve route-debug, target-output, printer/debug, same-block lookup,
  semantic resolver, control-flow, and store-source expectations unless the
  selected lookup path already owns that surface and the change is explicitly
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
- Run a broader backend/prepared subset if the value-home helper is shared by
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
