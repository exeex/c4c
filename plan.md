# Active Plan: Route 7 Comparison Provenance Consumer

Status: Active
Source Idea: ideas/open/215_route7_comparison_provenance_consumer.md

## Purpose

Activate the narrow Route 7 comparison provenance consumer work described by
the source idea: move exactly one comparison provenance consumer to
route/prepared agreement while keeping AArch64 prepared branch behavior and
expected-string authority intact.

Goal: make one named comparison provenance consumer accept Route 7 evidence only
when it agrees with the prepared/AArch64 provenance it already trusts.

## Core Rule

Move one comparison provenance consumer only. Do not migrate branch policy,
condition suffix mapping, fused legality, hazards, emitted-register state,
final instruction ordering, final assembler rows, Route 8 return-chain work, or
generic route-index facades.

## Read First

- `ideas/open/215_route7_comparison_provenance_consumer.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- One selected comparison provenance consumer in
  `src/backend/mir/aarch64/codegen/comparison.cpp`.
- Existing Route 7 comparison/materialized-condition provenance helpers and
  prepared fallback helpers used by that consumer.
- Branch-control tests that already exercise absent, invalid,
  duplicate/conflict, mismatch, and prepared fallback behavior.
- Machine-printer and branch-control expected strings that must stay unchanged.

## Non-Goals

- Do not move AArch64 branch targets, branch suffix selection, fused-compare
  legality, hazards, emitted registers, final instruction order, final
  assembler rows, or branch-policy oracle behavior into Route 7.
- Do not rewrite machine-printer or branch-control expected strings as a
  baseline refresh.
- Do not downgrade supported paths to unsupported paths or weaken test
  contracts.
- Do not hide old prepared/generic fallback behavior behind a new route facade.
- Do not touch Route 8 return-chain ownership or introduce a generic
  route-index abstraction.

## Working Model

Route 7 may supply selected comparison provenance: comparison-producing
instructions, materialized-condition provenance, branch-condition provenance,
and fused-compare operand producer facts. Prepared/AArch64 facts remain
authoritative for branch emission and output policy. The selected consumer may
observe Route 7 only through an agreement check that preserves prepared or
generic BIR fallback for absent, invalid, duplicate/conflicting, and mismatched
Route 7 facts.

## Execution Rules

- Start by naming exactly one comparison provenance consumer and its existing
  prepared fallback path.
- Treat route/prepared disagreement as prepared provenance, not as a Route 7
  override.
- Treat absent, invalid, duplicate/conflict, and mismatched Route 7 facts as
  fallback or fail-closed behavior matching current semantics.
- Keep proof focused on provenance equality and fallback behavior, not branch
  policy cleanup.
- For code-changing steps, run a fresh build or targeted CTest proof chosen by
  the supervisor.
- Escalate to reviewer scrutiny if the diff changes expected strings, branch
  suffixes, final assembler rows, fused legality, branch targets, or broad
  comparison helper ownership.

## Steps

### Step 1: Pin the Selected Consumer

Goal: identify the exact comparison provenance consumer to move and the
fixtures that prove it without changing implementation.

Primary target: `src/backend/mir/aarch64/codegen/comparison.cpp`.

Actions:

- Inspect the current comparison provenance readers around fused-compare,
  materialized-condition, and branch-condition provenance.
- Choose one named consumer and map it to its prepared or generic BIR fallback.
- Map the positive, absent, invalid, duplicate/conflict, mismatch, fallback,
  machine-printer, and branch-control proof points into `todo.md`.
- Identify the minimal backend test subset the supervisor should delegate for
  the implementation packet.
- Do not edit implementation while the selected consumer or proof surface is
  ambiguous.

Completion check:

- `todo.md` names one consumer, its prepared fallback, the route/prepared
  agreement point, the expected-string owners, and the minimal proof subset.

### Step 2: Add Route/Prepared Agreement

Goal: let the selected consumer use Route 7 provenance only when Route 7 and
prepared/AArch64 provenance agree.

Primary target: `src/backend/mir/aarch64/codegen/comparison.cpp`.

Actions:

- Thread only the data needed to query the selected Route 7 provenance for the
  named consumer.
- Compare the Route 7 answer against the existing prepared or generic BIR
  provenance answer before accepting it.
- Preserve current prepared or generic BIR fallback for absent Route 7 facts.
- Fail closed or fall back for invalid references, duplicate/conflicting facts,
  and route/prepared mismatches.
- Avoid moving suffix mapping, target selection, fused legality, final record
  ordering, final assembler behavior, or branch-policy decisions.

Completion check:

- The selected consumer receives the same provenance in the positive
  route/prepared agreement case and preserves current behavior for all
  fallback cases.

### Step 3: Prove Negative and Fallback Cases

Goal: prove the selected consumer is not testcase-shaped and keeps prepared
branch behavior authoritative.

Primary target: `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`.

Actions:

- Cover missing Route 7 facts with prepared or generic BIR fallback.
- Cover invalid references with the current fail-closed behavior.
- Cover duplicate or conflicting Route 7 facts without changing prepared or
  AArch64 branch behavior.
- Cover route/prepared mismatch with prepared provenance preserved.
- Check that branch policy, fused legality, suffixes, targets, and final
  assembler behavior are not altered.

Completion check:

- The targeted branch-control proof covers positive, absent, invalid,
  duplicate/conflict, mismatch, and fallback behavior without expected-string
  weakening.

### Step 4: Prove Expected-String Stability

Goal: show the provenance consumer change does not change printer/debug or
branch-control output authority.

Primary targets:

- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`

Actions:

- Run the delegated proof that machine-printer expected strings are unchanged.
- Run the delegated proof that branch-control expected strings are unchanged.
- Treat any expected-string baseline refresh, unsupported downgrade, or weaker
  contract as a blocker unless the supervisor explicitly approves it.

Completion check:

- Machine-printer and branch-control expected strings remain byte-stable under
  the selected Route 7 provenance consumer change.

### Step 5: Acceptance Handoff

Goal: leave the slice ready for supervisor acceptance or reviewer scrutiny.

Actions:

- Update `todo.md` with the final proof commands and results.
- Confirm the source idea remained unchanged.
- Confirm implementation changes are limited to the selected consumer and
  targeted tests.
- Flag branch-policy migration, expected-string rewrites, or Route 8/generic
  route-index work as blockers.

Completion check:

- The slice has fresh targeted proof, no testcase-overfit signs, and no
  branch-policy, final-assembler, suffix-mapping, fused-legality, or Route 8
  migration.
