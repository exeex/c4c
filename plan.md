# LIR Type Ref Structured Equality Runbook

Status: Active
Source Idea: ideas/open/176_lir_type_ref_structured_equality.md

## Purpose

Move one bounded `LirTypeRef` equality or comparison path away from rendered
text as semantic authority when structured payload, especially `StructNameId`,
is available.

Goal: make equal rendered LIR type text insufficient to mask missing or
mismatched structured identity metadata in the selected LIR path.

## Core Rule

Do not treat equal rendered `LirTypeRef` spelling as sufficient equality for
metadata-rich LIR when structured ids are present.

## Read First

- `ideas/open/176_lir_type_ref_structured_equality.md`
- `LirTypeRef` definition, parser, printer, and `operator==` or nearby
  comparison helpers.
- Existing frontend LIR and backend handoff tests that cover struct type refs,
  mirror validation, generated LIR validation, or metadata-bearing routes.
- Recent HIR structured identity work only as context; do not reopen idea 175.

## Current Targets

- One bounded `LirTypeRef` equality or comparison path where rendered text still
  decides equality despite structured metadata.
- Prefer `StructNameId` participation if the structured id is already available
  in generated LIR or a nearby verifier path.
- Focused frontend LIR or backend tests should prove equal text with missing or
  mismatched structured ids is observable.

## Non-Goals

- Do not retire every LIR text field in one slice.
- Do not change final rendered LIR syntax merely to expose ids.
- Do not replace all backend type lowering routes at the same time.
- Do not synthesize structured ids from rendered names at the comparison site.
- Do not expand into template record owner identity; that belongs to idea 177.

## Working Model

- Rendered LIR type text remains useful for display, legacy fixtures, printer
  stability, and diagnostics.
- Structured ids should be authoritative in metadata-rich comparisons when both
  sides carry them.
- Legacy or hand-authored no-id inputs may keep compatibility behavior only
  through an explicit and narrow rule.
- Missing metadata in a generated metadata-rich path should be observable
  through equality, verifier, route, or unit-test behavior instead of silently
  passing because text matches.

## Execution Rules

- Start by choosing exactly one LIR comparison surface and record it in
  `todo.md` before implementation.
- Keep changes local to `LirTypeRef` equality/comparison and any focused test
  harness needed to observe the structured-id collision.
- Preserve printer and parser output unless the selected proof route requires a
  deliberate metadata-observability change.
- Prefer a semantic helper name that identifies the structured payload being
  compared.
- Each code-changing step needs fresh build or compile proof plus targeted
  frontend LIR or backend CTest coverage chosen by the supervisor.

## Step 1: Select The LIR Comparison Surface

Goal: identify the smallest `LirTypeRef` equality or comparison path where
rendered text still controls semantic behavior.

Primary targets:

- Search for `LirTypeRef::operator==`, text-only `LirTypeRef` comparisons, and
  uses that affect dedup, mirror checks, generated LIR validation, or backend
  handoff.
- Inspect existing tests for struct type refs, `StructNameId`, frontend LIR
  generation, and backend route validation.

Concrete actions:

- Choose one comparison surface with structured metadata already available.
- Record the selected surface, files to inspect first, and focused proof command
  in `todo.md`.
- Identify the exact old comparison that must stop accepting equal text as
  sufficient for metadata-rich LIR.
- Confirm the route is not template record owner identity from idea 177.

Completion check:

- `todo.md` names the selected LIR comparison surface, initial files, and the
  narrow test bucket for the first implementation packet.

## Step 2: Add Structured Equality Probe Coverage

Goal: make equal rendered text with missing or mismatched structured ids
observable for the selected path.

Concrete actions:

- Add or extend focused tests so mismatched `StructNameId` payloads cannot
  compare equal only because rendered type text matches.
- Include a compatibility case for intentional legacy/no-id behavior if the
  selected path still supports it.
- Assert equality, verifier, route, or backend handoff behavior directly; do
  not rely only on printed text.

Completion check:

- The test would fail if the selected metadata-rich comparison still used only
  rendered `LirTypeRef` text.

## Step 3: Compare Structured Payload

Goal: update the selected LIR path so structured payload participates in
metadata-rich equality.

Concrete actions:

- Reuse existing structured id fields such as `StructNameId`; do not reconstruct
  ids from rendered names at the comparison site.
- Separate display text from semantic equality in code shape.
- Make no-id compatibility explicit and narrow.
- Add a focused comment only where the compatibility rule would otherwise be
  unclear.

Completion check:

- Equal rendered text no longer compares equal in the selected metadata-rich
  path when structured ids are missing or mismatched.

## Step 4: Validate The Focused Slice

Goal: prove the selected LIR comparison improved without unrelated printer,
parser, or backend drift.

Concrete actions:

- Run the delegated build or compile proof.
- Run targeted frontend LIR or backend CTest coverage for the selected path.
- Re-run adjacent tests if the implementation touches shared equality helpers
  used by more than one LIR route.

Completion check:

- Build or compile proof is fresh.
- Targeted frontend LIR or backend tests pass.
- Existing display/printer behavior remains stable unless deliberately changed
  and covered.

## Step 5: Decide Whether The Idea Is Complete

Goal: decide whether the selected comparison surface satisfies idea 176 or
whether another LIR equality path needs a follow-up runbook.

Concrete actions:

- Compare the accepted implementation against the source idea acceptance
  criteria.
- If the selected path is complete but important LIR equality surfaces remain
  uncovered, leave durable follow-up notes in `todo.md` and ask the supervisor
  to route lifecycle review.
- If the source idea itself is satisfied, run the close gate through the
  lifecycle workflow before closing.

Completion check:

- The supervisor has enough proof notes to continue execution, request plan
  review, or close the source idea through the regression-guarded lifecycle
  gate.
