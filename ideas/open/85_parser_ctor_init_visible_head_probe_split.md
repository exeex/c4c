# Parser Constructor-Init Visible-Head Probe Split

Status: Open
Last Updated: 2026-04-23

## Goal

Split the constructor-init visible-head ambiguity into smaller parser
seams so the `source(other)` regression family can be understood and
repaired without repeatedly re-running the same broad step-2 packet.

## Why This Idea Exists

Repeated packets on the current parser runbook collide on the same
constructor-init probe boundary:

- moving the visible-head classifier earlier regresses
  `Box value(source(other));`
- restoring the prior ordering makes the narrow parser tests pass again
- the failure family does not shrink when grouped pointer/reference cases are
  retried as one bundle

That means the current route is too coarse. The ambiguity needs to be split
into focused seams before more implementation work can be made durable.

## Main Objective

Separate the constructor-init visible-head decision point into explicit,
independently testable parser seams.

The intended direction is:

- keep parenthesized local-value direct-init classification distinct from
  grouped pointer/reference starter handling
- keep unresolved grouped pointer/reference parameter-style starters distinct
  from the visible-head handoff boundary
- make the boundary between
  `classify_visible_value_or_type_head()` and later call-like starter checks
  explicit in the parser probe flow
- prove each ambiguity seam with focused parser regressions instead of one
  broad syntactic bucket

## Primary Scope

- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser.hpp`
- `tests/frontend/frontend_parser_tests.cpp`
- narrower focused parser/front-end probe additions if needed for a seam

## Seams To Split Out

1. Parenthesized local-value direct-init classification.
   - `identifier(...)` forms with a visible value head
   - the `Box value(source(other));`-style path

2. Unresolved grouped pointer/reference parameter-style starters.
   - `identifier &/* identifier`
   - grouped `identifier((...))` forms

3. The handoff boundary between visible-head classification and later starter
   checks.
   - `classify_visible_value_or_type_head()`
   - later call-like starter probes in constructor-init handling

4. Focused regressions for each seam.
   - one regression per ambiguity seam
   - no expectation downgrade as the primary proof mechanism

## Non-Goals

- no lexical-scope lookup migration work
- no namespace traversal refactor
- no broad parser cleanup unrelated to the constructor-init ambiguity
- no testcase-shaped overfit that only patches `source(other)`
- no expectation downgrades without explicit approval

## Working Model

- treat the current regression as a probe-boundary problem, not a generic
  parser failure
- keep the constructor-init decision path decomposed into small, named seams
- validate each seam with a focused parser test before trying to recombine
  them
- if a seam cannot be isolated cleanly, record the blocking ambiguity rather
  than widening the route

## Execution Rules

- preserve existing passing behavior for `Box value(source(other));`
- do not generalize the visible-head classifier change until the failing seam
  is explained
- keep the probe split narrow enough that each step has an observable failure
  family
- prefer build proof plus focused parser tests after each code change
- widen validation only if the parser blast radius grows beyond the
  constructor-init probe surface

## Steps

1. Map the constructor-init visible-head failure family.
   - Reproduce the current `source(other)` regression boundary.
   - Record the exact ambiguity seam in `todo.md`.
   - Completion check: the known collision is split into named subcases.

2. Separate the visible-head handoff from the constructor-init probe.
   - Make the classifier/probe boundary explicit enough that the handoff can
     be reasoned about independently.
   - Preserve the passing `Box value(source(other));` path.
   - Completion check: the handoff is expressed as a smaller, testable
     decision point.

3. Add focused parser regressions for each seam.
   - Add or narrow tests for parenthesized local-value direct-init,
     grouped pointer/reference starters, and handoff behavior.
   - Completion check: each seam has at least one explicit focused regression.

4. Re-run focused parser proof and stop if the failure family expands.
   - Build the frontend and run the narrow parser subset needed for the seams.
   - Completion check: the updated seam split passes its focused proof.

## Follow-On Relationship

This idea intentionally splits from:

- `ideas/open/83_parser_scope_textid_binding_lookup.md`

The boundary is deliberate:

- idea 83: broader lexical-scope lookup migration
- idea 85: constructor-init visible-head ambiguity decomposition

