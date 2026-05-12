# HIR TypeSpec Ref Structured Equivalence Runbook

Status: Active
Source Idea: ideas/open/175_hir_typespec_ref_structured_equivalence.md

## Purpose

Move one ordinary HIR type-reference path away from raw or partial `TypeSpec`
equivalence as semantic identity, while preserving `TypeSpec` where it is still
syntax, declarator, unresolved-array, or display payload.

Goal: pick one bounded HIR type-ref family and make equality, lookup, binding,
or dedup use structured resolved identity when those facts are available.

## Core Rule

Do not treat equal rendered spelling or broad `TypeSpec` shape as sufficient
semantic identity in the selected metadata-rich HIR path.

## Read First

- `ideas/open/175_hir_typespec_ref_structured_equivalence.md`
- Existing HIR/frontend tests around member typedef, parser type metadata,
  ordinary declaration lowering, and HIR type-reference metadata.
- The selected implementation surface before editing, especially any helper
  that currently compares `TypeSpec`, rendered tags, typedef names, record
  tags, or template-owner payloads.

## Current Targets

- One ordinary HIR type-ref family where raw or partial `TypeSpec` equivalence
  still decides semantic behavior.
- Good candidate families include member typedef binding/readiness, ordinary
  declaration or parameter type refs, record/typedef owner lookup, or array /
  function-pointer type refs if the structured facts are already available.
- Focused frontend/HIR tests under `tests/frontend/` or a nearby HIR fixture
  should prove the chosen collision-prone domain.

## Non-Goals

- Do not migrate every `TypeSpec` field in HIR.
- Do not rewrite the full parser, sema, or HIR type lowering pipeline.
- Do not remove display formatting, diagnostics, dump text, syntax payload,
  declarator state, or unresolved array expressions just because they use
  `TypeSpec`.
- Do not add named-case matching for one typedef, record, array, or function
  pointer spelling.
- Do not weaken tests or recategorize supported behavior as unsupported.

## Working Model

- `TypeSpec` can still be the right container for spelling, declarator shape,
  qualifiers, syntax recovery, unresolved dimensions, and display context.
- Structured identity should come from existing sema/HIR facts: canonical type
  payload, record owner keys, typedef origin metadata, callable signature
  structure, template binding metadata, or another already-threaded identity
  carrier.
- Legacy/no-metadata behavior must be explicit. A compatibility path may keep
  old behavior only when structured identity is genuinely unavailable, and the
  selected metadata-rich path must not silently fall back to rendered equality.

## Execution Rules

- Start by choosing exactly one HIR type-ref family; record the choice in
  `todo.md` before code changes.
- Keep changes local to the selected family and any small helper needed to make
  structured comparison readable.
- Prefer a semantic helper with a name that says which structured identity is
  authoritative; avoid generic helper renames that leave the old comparison in
  place.
- Preserve existing printer and diagnostic output unless a testable observably
  wrong fallback requires a deliberate change.
- Add focused tests before or with the implementation so equal-spelling
  collisions cannot keep passing unnoticed.
- Each code-changing step needs fresh build or compile proof plus targeted
  frontend/HIR CTest coverage chosen by the supervisor.

## Step 1: Select The HIR Type-Ref Family

Goal: identify the smallest ordinary HIR type-ref family where raw or partial
`TypeSpec` comparison still controls semantic behavior.

Primary targets:

- Search HIR/frontend code for `TypeSpec` equality, rendered-name comparison,
  typedef tag comparison, record tag comparison, and fallback owner matching.
- Inspect nearby tests that already construct stale rendered tags, owner
  metadata, typedef origins, arrays, function pointers, or record/template
  owners.

Concrete actions:

- Choose one family with existing structured facts already available.
- Record the selected family, implementation surface, and focused proof command
  in `todo.md`.
- Identify the exact old equality, lookup, binding, or dedup decision that must
  stop using raw or partial `TypeSpec` equivalence.
- Confirm the route is not actually idea 176 LIR equality or idea 177 template
  record owner identity.

Completion check:

- `todo.md` names the selected HIR family, the files to inspect first, and the
  narrow test bucket the executor should use for the first implementation
  packet.

## Step 2: Add Structured Identity Probe Coverage

Goal: make the selected collision observable before the semantic change lands,
or add the test alongside the implementation if the current harness cannot
express the failure in isolation.

Concrete actions:

- Add or extend focused frontend/HIR tests so equal rendered spelling with
  missing or mismatched structured identity is not silently accepted.
- Include a nearby non-collision compatibility case for legacy/no-metadata
  inputs if the selected path still needs fallback behavior.
- Keep assertions about structured identity, lookup result, binding result, or
  dedup behavior; do not rely only on printed text.

Completion check:

- The test describes the chosen collision-prone family and would fail if the
  selected path still accepts equal raw `TypeSpec` spelling as semantic
  equality in metadata-rich cases.

## Step 3: Thread Or Compare Structured Identity

Goal: update the selected HIR path so structured resolved identity decides the
metadata-rich case.

Concrete actions:

- Reuse existing sema/HIR identity carriers where available instead of
  inventing rendered-name reconstruction at the comparison site.
- Separate syntax/display payload from semantic identity in code shape.
- Make no-metadata compatibility explicit and narrow.
- Add a focused comment only if the compatibility rule would otherwise be
  unclear.

Completion check:

- The selected equality, lookup, binding, or dedup point rejects or reports
  mismatched structured identity even when rendered `TypeSpec` spelling would
  match.

## Step 4: Validate The Focused Slice

Goal: prove the selected HIR family improved without broad type-system drift.

Concrete actions:

- Run the delegated build or compile proof.
- Run targeted frontend/HIR CTest coverage for the selected family.
- Re-run adjacent tests if the implementation touched shared helpers used by
  multiple ordinary HIR type refs.

Completion check:

- Build or compile proof is fresh.
- Targeted frontend/HIR tests pass.
- Existing display, diagnostics, and syntax/declarator behavior remain intact.

## Step 5: Decide Whether The Idea Is Complete

Goal: decide whether one focused family satisfies idea 175 or whether a
follow-up runbook is needed for another ordinary HIR type-ref family.

Concrete actions:

- Compare the accepted implementation against the source idea acceptance
  criteria.
- If the selected family is complete but the source idea still has important
  uncovered HIR families, leave durable follow-up notes in `todo.md` and ask
  the supervisor to route lifecycle review.
- If the source idea itself is satisfied, ask the plan owner to run the close
  gate; do not close only because this runbook is exhausted.

Completion check:

- The supervisor has enough proof notes to either continue execution, request a
  plan review, or close the source idea through the lifecycle gate.
