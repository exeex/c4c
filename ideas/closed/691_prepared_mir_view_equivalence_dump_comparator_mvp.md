# Prepared MIR View Equivalence Dump Comparator MVP

Status: Closed
Closed: 2026-07-11
Type: Implementation idea
After: `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
Parent: `ideas/closed/683_prepared_mir_view_contract_research.md`
Consumes:
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`
Related:
- `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/prepared_view.cpp`
Owning Layer: Prepared MIR view proof boundary

## Goal

Add the first canonical prepared MIR view dump and an old-vs-old structural
comparator MVP for view-exposed facts, so future producer and target
migrations can prove equivalence without freezing the full
`PreparedBirModule`.

## Why This Exists

Idea 684 established the first real MIR-facing prepared view and x86 consumer
migration. The next migration risk is proof quality: future RV64, AArch64, or
new-producer work needs a typed comparison surface that checks the view
contract rather than relying on broad prepared internals, textual debug dumps,
or object-output-only smoke tests.

## In Scope

- Define a canonical dump for schema, target identity, functions, cursors,
  core prepared facts, selected feature-presence summaries, and verifier
  statuses already exposed through the prepared MIR view.
- Add an old-route view-vs-view structural comparator skeleton.
- Prove old-vs-old equality on representative x86 prepared modules.
- Add at least one intentional-difference test if a small deterministic fixture
  can be built without broad producer rewrites.
- Keep diagnostic notes, route names, completed phases, proof certificates,
  and rendered debug text out of equality.

## Out Of Scope

- Replacing the BIR producer or requiring a new producer.
- Comparing every optional feature family in the first comparator slice.
- Changing MIR, assembly, object output, runtime behavior, diagnostics,
  expectations, unsupported markers, allowlists, timeout policy, or baseline
  acceptance policy.
- Freezing the complete `PreparedBirModule` layout as a public compatibility
  contract.
- Migrating RV64 or AArch64 target consumers.

## Acceptance Criteria

- A deterministic prepared MIR view dump exists for the first core view shape.
- A structural comparator can compare two old-route views and report equality
  for representative current modules.
- Equality is based on typed view facts, not raw prepared-module internals or
  rendered debug text.
- Focused tests cover the dump and comparator behavior.
- Fresh build plus focused backend/comparator tests pass.

## Closure Summary

The MVP is complete. `PreparedMirCoreView` now exposes a deterministic
canonical dump and a typed structural comparison path for the current core view
shape. The focused comparator test target covers deterministic dump output,
diagnostic/prepared-history exclusion, old-route view equality, and a typed
blocking difference for core fact divergence. Test routing now selects both
prepared MIR proof targets with `^backend_prepared_mir_`.

Boundary audit outcome:
- no expectation rewrites, unsupported-marker downgrades, allowlist edits, or
  target behavior changes were used as proof
- comparator equality is based on typed view snapshots rather than rendered
  dump text
- full `PreparedBirModule` compatibility is not frozen as the public proof
  contract
- richer source/freshness dependency authority remains in
  `ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md`

Close proof:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS with 2/2 focused prepared MIR tests passing before and after.

## Reviewer Reject Signals

- Reject a comparator that serializes or compares the whole
  `PreparedBirModule` under a new name.
- Reject equality based primarily on route text, diagnostic notes, completed
  phases, proof certificates, or pretty-printed debug output.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  baseline-policy changes claimed as comparator progress.
- Reject broad BIR/prealloc rewrites or new-producer work hidden inside this
  MVP.
- Reject named-case-only fixtures that do not exercise the typed view facts the
  comparator claims to own.
- Reject target behavior changes used as proof of a dump/comparator-only
  slice.
