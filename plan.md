# Prepared MIR View Equivalence Dump Comparator MVP Plan

Status: Active
Source Idea: ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md

## Purpose

Create the first proof surface for the prepared MIR view boundary: a
deterministic canonical dump plus an old-route view-vs-view structural
comparator for view-exposed facts.

## Goal

Let future producer and target migrations prove equality at the
`PreparedMirCoreView` boundary without comparing the whole
`prepare::PreparedBirModule`, rendered debug text, target output, or diagnostic
history.

## Core Rule

Compare typed prepared MIR view facts first. Textual dumps are supporting
debug aids only, and equality must not depend on route names, completed phases,
prepare notes, proof text, pretty-printer output, expectations, unsupported
markers, allowlists, or target behavior changes.

## Read First

- `ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md`
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`
- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/prepared_view.cpp`
- `tests/backend/mir/backend_prepared_mir_core_view_test.cpp`

## Current Targets

- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/prepared_view.cpp`
- `tests/backend/mir/CMakeLists.txt`
- new or existing focused tests under `tests/backend/mir/`

## Non-Goals

- Do not replace the BIR producer or require a new producer.
- Do not compare the full `PreparedBirModule` under a new name.
- Do not migrate RV64, AArch64, or additional target consumers.
- Do not change MIR lowering, assembly, object output, runtime behavior,
  diagnostics, expectations, unsupported markers, allowlists, timeout policy,
  or baseline acceptance policy.
- Do not make route text, diagnostic notes, completed phases, proof
  certificates, or rendered debug output part of structural equality.

## Working Model

- The old-route producer builds `prepare::PreparedBirModule`.
- `PreparedMirCoreView` projects only the MIR-facing facts needed by the core
  view contract.
- A canonical dump renders a deterministic, parser-friendly view of that
  projection for humans and golden tests.
- A structural comparator compares typed snapshots or direct view accessors and
  returns classified differences.
- The first comparator compares old-route view against old-route view. Future
  old-vs-new producer work can reuse the same contract.

## Execution Rules

- Keep the first dump and comparator narrow to the current core view shape:
  target identity, function names, defined-function traversal, module data,
  per-function control flow, value locations, stack layout/addressing presence,
  block bindings, instruction cursors, and view-owned lookup-visible facts.
- Sort output by stable ids or traversal order already exposed by the view.
- Use typed difference records before string output. Human-readable messages
  may summarize the record but must not be the comparator key.
- If an intentional-difference fixture would require broad producer mutation,
  use a small deterministic in-memory fixture instead.
- Keep new helpers in the MIR prepared-view boundary unless an existing local
  helper pattern clearly owns them.
- For code-changing steps, prove with a fresh build and focused CTest target.
  Broader validation belongs to the supervisor.

## Ordered Steps

### Step 1: Add Canonical Core View Dump

Goal: expose a deterministic textual dump of the current
`PreparedMirCoreView` core projection.

Actions:
- Add a dump API that renders only view-exposed core facts.
- Include schema/version, target identity, functions, defined-function
  traversal, globals/string constants, block bindings, instruction cursors,
  value-location summaries, addressing/control-flow presence, and lookup-owned
  facts that are already exposed through the view.
- Exclude prepare notes, completed phases, private route names, proof text,
  diagnostic wording, target route summaries, and final MIR/object output.
- Add focused dump tests using the existing core-view fixture or a similarly
  small deterministic fixture.

Completion Check:
- A focused test proves deterministic dump content and proves at least one
  excluded diagnostic/prepared-history field is absent.
- Build plus the focused prepared MIR view test target pass.

### Step 2: Add Typed Structural Snapshot And Comparator Skeleton

Goal: compare two old-route prepared MIR core views through typed view facts.

Actions:
- Define comparator options and a report type for equality, blocking
  required-fact differences, presence differences, diagnostic-only
  differences if needed, and comparator-unsupported outcomes.
- Build a typed snapshot or direct accessor comparison for the same core facts
  covered by Step 1.
- Keep string dump comparison out of the equality decision.
- Make raw `PreparedBirModule` internals inaccessible to comparator callers
  except through `PreparedMirCoreView`.

Completion Check:
- Old-vs-old equality passes for representative x86 prepared modules.
- A small intentional-difference fixture reports a typed blocking difference
  without relying on rendered text.

### Step 3: Integrate Focused Tests And Build Routing

Goal: make the dump and comparator proof surface available to normal backend
test workflows.

Actions:
- Register the focused test target in `tests/backend/mir/CMakeLists.txt`.
- Keep target naming specific enough for the supervisor to route a narrow
  CTest subset.
- Add assertions that comparator equality ignores diagnostic-only prepared
  history and fails on core fact divergence.
- Preserve the existing `backend_prepared_mir_core_view` behavior.

Completion Check:
- Fresh build of the new focused target passes.
- `ctest --test-dir build -R '^backend_prepared_mir_' --output-on-failure`
  exercises the new comparator/dump tests and existing core-view test.

### Step 4: Boundary Audit And Handoff Notes

Goal: leave clear evidence that the MVP satisfies idea 691 without expanding
  into producer replacement or freshness-view work.

Actions:
- Audit the diff for raw prepared-module comparison, diagnostic-string
  equality, expectation rewrites, unsupported-marker changes, allowlist edits,
  and target behavior changes.
- Record proof commands and any unsupported comparator feature gaps in
  `todo.md`.
- If source/freshness facts are needed for richer comparison, leave them to
  `ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md`.

Completion Check:
- `todo.md` records the focused proof results and any follow-up notes.
- The active idea is ready for lifecycle close decision or an explicitly
  justified next comparator step.
