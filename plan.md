# Prepared MIR Source Dependency Freshness View Contract Plan

Status: Active
Source Idea: ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md

## Purpose

Implement the first typed Prepared MIR source/freshness view contract so MIR
consumers can read selected lowering authority without rediscovering it from
raw prepared internals.

## Goal

Expose one narrow, producer-backed source/freshness authority family through
`src/backend/mir/prepared_view.*`, prove accepted and rejected paths, and keep
diagnostic or explanatory proof state out of semantic lowering authority.

## Core Rule

Only valid producer facts may become lowering authority. Missing, stale,
ambiguous, wrong-use, wrong-value, destination-only, debug, route, and
explanation state must remain verifier, admission, or diagnostic data.

## Read First

- `ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md`
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`
- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/prepared_view.cpp`
- `src/backend/mir/x86/`

## Scope

- Select one already-governed source/freshness authority family, preferably
  from the direct edge-publication or branch stack-load freshness contracts.
- Add a typed view surface that exposes selected source identity and freshness
  only after admission has accepted the producer fact.
- Represent rejected or incomplete authority as fail-closed verifier/admission
  state, not as source values that target lowering can consume.
- Migrate one low-risk x86 consumer path or add structural tests when a target
  migration would be too broad for the first slice.

## Non-Goals

- Do not reopen ideas 589 or 590.
- Do not replace all publication or source dependency producers.
- Do not migrate all targets.
- Do not delete broad raw prepared fields.
- Do not treat stack homes, move bundles, destination legality, route names,
  rendered debug text, diagnostics, expectations, allowlists, timeouts, or
  baseline policy as source/freshness authority.

## Working Model

The core Prepared MIR view stays reference-only over the prepared BIR module.
This plan adds a typed feature view layered on top of selected prepared facts.
The feature view should make legal source/freshness facts easy for target code
or comparators to consume while making rejected facts impossible to mistake for
lowering inputs.

## Execution Rules

- Keep edits behavior-preserving until the selected consumer path is migrated.
- Prefer focused adapter/comparator tests before broad target changes.
- If no legal first authority family is available, record the evidence in
  `todo.md` and ask for lifecycle review instead of inventing authority.
- Do not infer authority from raw publication row shape or debug strings.
- Do not weaken expectations, unsupported markers, diagnostics, timeout
  handling, or pass/fail accounting as proof of progress.
- For code-changing steps, use a fresh build plus focused backend tests; the
  supervisor decides whether broader regression proof is required.

## Ordered Steps

### Step 1: Select Source/Freshness Authority Family

Goal: identify the first legal authority family for the view.

Primary targets:

- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/x86/`

Actions:

- Inspect the closed freshness contracts and current prepared view API.
- Choose one source/freshness family with existing producer ownership and a
  narrow consumer or structural proof path.
- Record accepted states, rejected states, and the consumer program point or
  test surface.
- Leave broad producer rewrites and multi-target migration out of this step.

Completion check:

- `todo.md` names the selected family, accepted lowering inputs, rejected
  admission states, and focused proof command for Step 2.

### Step 2: Add Typed Freshness View Contract

Goal: implement the reference-only feature view for the selected family.

Primary targets:

- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/prepared_view.cpp`
- focused backend MIR tests

Actions:

- Add typed value/source/freshness accessors or result structs for the selected
  family.
- Ensure accepted results are distinct from missing, stale, ambiguous,
  wrong-use, wrong-value, or destination-only states.
- Keep route names, proof paths, debug summaries, and checker explanations out
  of semantic return values.
- Add focused tests that cover one accepted fact and at least one rejected fact.

Completion check:

- Fresh build and focused backend MIR tests pass.
- The tests prove rejected authority cannot be consumed as lowering authority.

### Step 3: Migrate One Low-Risk x86 Consumer Or Comparator Path

Goal: prove the feature view is usable by one real MIR-facing consumer surface.

Primary targets:

- `src/backend/mir/x86/`
- `src/backend/mir/prepared_view.*`
- focused backend tests or comparator tests

Actions:

- Replace one raw prepared dependency read with the typed view when the
  selected family is admitted.
- Preserve fail-closed behavior when the view reports missing or invalid
  authority.
- Keep all other target paths and producer internals unchanged unless the
  selected consumer requires a tiny local adapter.

Completion check:

- The migrated path no longer takes source/freshness authority from raw
  prepared internals.
- Fresh build plus focused backend tests pass.

### Step 4: Audit Boundaries And Close Readiness

Goal: verify the first source/freshness view contract satisfies the source
idea without hiding follow-up work.

Actions:

- Audit the diff for raw publication rows, stack homes, move bundles, route
  names, debug text, or diagnostics being used as lowering authority.
- Confirm accepted and rejected paths are covered by focused proof.
- Record any remaining source/freshness families as follow-up scope rather
  than expanding this first contract.

Completion check:

- `todo.md` records closure evidence, focused proof, and any follow-up ideas
  needed outside this source idea.
