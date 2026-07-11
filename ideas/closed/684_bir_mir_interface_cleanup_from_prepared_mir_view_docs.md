# BIR MIR Interface Cleanup From Prepared MIR View Docs

Status: Closed
Type: Implementation idea
After: `ideas/closed/683_prepared_mir_view_contract_research.md`
Parent: `ideas/closed/683_prepared_mir_view_contract_research.md`
Consumes:
- `docs/prepared_mir_view_contract_research/index.md`
- `docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md`
- `docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`
- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`
- `docs/prepared_mir_view_contract_research/06_incremental_migration_plan_for_mir_consumers.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`
Related:
- `ideas/closed/678_lir_to_bir_adapter_boundary_umbrella.md`
- `ideas/closed/683_prepared_mir_view_contract_research.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/`
- `src/backend/mir/`
- `src/backend/x86/`
- `src/backend/riscv/`
- `src/backend/aarch64/`
Owning Layer: BIR/prepared to MIR consumer interface boundary

## Closure Summary

Closed on 2026-07-11 after the first behavior-preserving prepared-to-MIR
interface cleanup landed.

Completed implementation:

- added a reference-only `PreparedMirCoreView` / `PreparedMirFunctionView`
  adapter over the current `prepare::PreparedBirModule`;
- routed the x86 internal module-emission path through the prepared MIR view
  while keeping the public compatibility wrapper stable;
- moved the selected x86 local-slot return consumption behind
  `PreparedMirFunctionView`;
- added `scripts/x86_raw_prepared_dependency_guard.py` to make raw
  `PreparedBirModule` and broad prealloc dependencies visible under migrated
  x86 MIR surfaces.

Close proof used the focused x86/backend scope recorded in `test_before.log`
and `test_after.log`: both logs show 78/78 tests passing, and the regression
guard passes in non-decreasing mode because this slice is behavior-preserving.
The raw dependency guard reports only classified existing hits.

Follow-up source ideas were opened for the next work instead of expanding this
first cleanup slice:

- `ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md`
- `ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md`

## Goal

Implement the first behavior-preserving BIR-to-MIR interface cleanup described
by the prepared MIR view research docs. The intended outcome is a narrow
`PreparedMirView` or equivalent MIR-facing adapter that lets MIR consumers read
required prepared facts without depending directly on the full
`PreparedBirModule`.

## Why This Exists

The BIR and prepared/prealloc layers are currently difficult to refactor
because target MIR consumers can observe broad prepared-module state. That
coupling makes the future BIR rewrite riskier: a new BIR producer would have to
recreate legacy prepared internals rather than satisfy a narrow MIR input
contract.

Idea 683 exists to decide the contract shape first. This implementation idea
starts only after those docs exist, then consumes the research output and lands
the smallest useful interface cleanup slice.

## In Scope

- Read all required documents under
  `docs/prepared_mir_view_contract_research/` before changing source.
- Add the first MIR-facing prepared view or adapter surface named by the docs.
- Keep the first slice behavior-preserving: existing MIR output, object output,
  runtime behavior, and diagnostics should remain unchanged.
- Migrate one low-risk MIR consumer path first, preferably the x86-first path
  recommended by the research docs.
- Keep old direct `PreparedBirModule` access available for unmigrated
  consumers until their view migration is explicit and proved.
- Add focused compile or unit coverage for the new interface boundary when the
  slice introduces a separately testable adapter.
- Record any implementation notes under
  `docs/prepared_mir_view_contract_research/implementation_notes.md` only when
  the notes clarify research-to-code mapping.

## Out Of Scope

- Rewriting BIR or replacing the full BIR-to-prealloc pipeline.
- Implementing the full phoenix rebuild sequence from draft ideas 679-682.
- Deleting broad `PreparedBirModule` fields before migrated consumers no
  longer depend on them.
- Changing runtime behavior, target ABI classification, unsupported markers,
  allowlists, expectations, timeout policy, or baseline acceptance policy.
- Moving diagnostic-only proof facts into MIR codegen authority.
- Migrating every target in one slice.

## Required Starting Evidence

Before implementation starts, the active runbook must cite:

- which `PreparedMirView` core shape from `02_prepared_mir_core_view_shape.md`
  is being implemented;
- which current MIR dependency from
  `01_current_mir_dependencies_on_prepared_bir.md` is being narrowed;
- which migration order from
  `06_incremental_migration_plan_for_mir_consumers.md` is being followed;
- which diagnostic/proof facts from
  `04_debug_proof_and_diagnostic_boundaries.md` must remain observational.

## Acceptance Criteria

- The docs from idea 683 exist and are explicitly consumed by the active
  runbook before source edits begin.
- A first MIR-facing prepared view or adapter exists in source and is used by
  at least one real MIR consumer path.
- The migrated path no longer needs unrestricted direct access to the full
  `PreparedBirModule` for the facts covered by the new view.
- Unmigrated paths remain behavior-compatible through the old access path or a
  compatibility adapter.
- The proving command includes a fresh build plus a focused backend subset
  covering the migrated MIR consumer path.
- No test expectation, unsupported-marker, allowlist, runtime-output, or
  baseline-policy downgrade is used as proof of progress.

## Reviewer Reject Signals

- Reject implementation that starts before the required research docs from
  idea 683 exist.
- Reject a `PreparedMirView` that simply exposes the entire current
  `PreparedBirModule` under a new name.
- Reject target MIR code that keeps using unrestricted prepared-module access
  for the supposedly migrated fact family.
- Reject moving diagnostic, route-proof, freshness-debug, or local-array
  provenance artifacts into semantic codegen authority without explicit
  required-fact promotion from the research docs.
- Reject broad BIR/prealloc rewrites, phoenix-stage implementation, or legacy
  deletion hidden inside this first interface cleanup.
- Reject testcase-shaped shortcuts, named-case-only fixes, expectation
  rewrites, unsupported downgrades, allowlist filtering, or weaker proof
  commands claimed as interface cleanup.
- Reject a migration that changes MIR/object/runtime behavior without a
  documented compatibility reason and focused proof.
