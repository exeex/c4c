# Parser Disambiguation Matrix Runbook

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Activated from: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md

## Purpose

Turn parser declaration-versus-expression disambiguation around type-ids and
parenthesized declarators into an explicit matrix-driven test and fix loop.

## Goal

Build and use a generated parser-disambiguation matrix that exposes weak spots
systematically, then close the highest-value parser holes with narrow,
test-backed slices.

## Core Rule

Bias toward matrix coverage and targeted parser fixes. Do not absorb broader
language-completeness, inheritance, rvalue-reference, or backend work into this
plan unless a concrete parser-disambiguation step here cannot complete without
it.

## Read First

- `ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md`
- `scripts/generate_parser_disambiguation_matrix.py`
- `ideas/open/44_parser_disambiguation_matrix_patterns.txt`
- `ideas/open/44_parser_disambiguation_matrix_patterns_by_owner.txt`
- `ideas/open/44_parser_disambiguation_matrix_patterns_by_declarator.txt`

## Current Targets

- parser type-head recognition in ambiguous contexts
- parenthesized declarator forms such as `(*)`, `(&)`, `(&&)`, and `C::*`
- consistent declaration-versus-expression splitting across cast and local
  declaration contexts
- generated matrix coverage under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/`

## Non-Goals

- broad sema or HIR feature work unless directly required to keep a selected
  matrix case compiling through its intended validation tier
- unrelated inheritance, template materialization, backend, or EASTL bring-up
- hand-maintained one-off regression growth when the generator can express the
  family

## Working Model

1. Keep the matrix dimensions explicit: owner spelling, declarator family,
   parse context, validation tier.
2. Land test-surface improvements before parser behavior changes whenever a new
   family is being added.
3. Prefer `parse_only` breadth first; promote only selected high-risk families
   to stronger validation tiers.
4. Fix one parser root cause at a time and re-run the narrow family that proved
   the issue before broader validation.

## Execution Rules

- Keep generated tests separate from hand-written reductions.
- Preserve machine-readable metadata and deterministic generation inputs.
- When a case reveals a separate initiative, record it under `ideas/open/`
  instead of mutating this plan.
- Use Clang only as a syntax/behavior reference when the intended parse is
  unclear.
- Avoid broad parser rewrites without first pinning the exact matrix family and
  context that fail.

## Ordered Steps

### Step 1: Audit generator and existing matrix assets

Goal: confirm the current matrix inputs, emitted file layout, and coverage gaps
before adding or fixing cases.

Primary targets:

- `scripts/generate_parser_disambiguation_matrix.py`
- `ideas/open/44_parser_disambiguation_matrix_patterns*.txt`
- `tests/cpp/internal/generated/parser_disambiguation_matrix/`

Actions:

- inspect the generator inputs and outputs already present in tree
- identify which owner, declarator, and context families already exist
- record missing families and any drift between manifests and generated tests
- keep the first execution slice limited to matrix/test-surface setup unless a
  blocking parser bug is already explicitly covered

Completion check:

- the repo state makes it clear what the generator currently covers
- the next missing or failing matrix family is explicitly identified in
  `todo.md`

### Step 2: Stabilize parse-only matrix breadth

Goal: add or repair generated `parse_only` coverage for the most important
ambiguous families.

Primary targets:

- `scripts/generate_parser_disambiguation_matrix.py`
- `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`

Actions:

- add missing matrix rows for owner spellings and declarator families justified
  by the source idea
- keep emitted filenames and metadata deterministic and reviewable
- wire the generated cases into the existing test surface with the lightest
  validation needed for parser coverage

Completion check:

- the selected parse-only family is generated rather than hand-authored
- targeted parser-only validation runs against the new family

### Step 3: Fix parser split failures by family

Goal: repair declaration-versus-expression or type-head parsing failures
revealed by the matrix.

Primary targets:

- `src/frontend/parser/`
- any parser helper used for type-head or declarator recognition

Actions:

- choose one failing family at a time from the generated matrix
- add or keep the narrowest regression proving the exact split failure
- implement the smallest parser change that corrects that family
- re-run the affected family and nearby parser tests before moving on

Completion check:

- one concrete matrix family moves from failing to passing
- no unrelated family expansion is bundled into the same slice

### Step 4: Promote selected high-risk families beyond parse-only

Goal: strengthen a small number of matrix cases where syntax alone is not
sufficient.

Primary targets:

- `tests/cpp/internal/generated/parser_disambiguation_matrix/compile_positive/`
- `tests/cpp/internal/generated/parser_disambiguation_matrix/runtime_positive/`

Actions:

- promote only families called out by the source idea as high-risk
- keep runtime-positive generation template-driven
- require host compiler success before treating a promoted runtime case as
  finished

Completion check:

- at least one high-risk family has stronger-than-parse-only validation
- stronger validation remains bounded and template-driven

### Step 5: Close with manifest-backed coverage notes

Goal: leave the matrix surface auditable and resumable for future parser work.

Actions:

- ensure manifests and generated tests still agree
- record any deferred adjacent initiatives back into `ideas/open/` if needed
- keep `todo.md` aligned with the last completed family and next intended slice

Completion check:

- another agent can see what matrix coverage exists, what was completed, and
  what family should be tackled next
