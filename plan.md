# Active Plan: Route 7 Comparison Oracle Row

Status: Active
Source Idea: ideas/open/216_route7_comparison_oracle_row.md

## Purpose

Activate the narrow Route 7 diagnostic/oracle replacement described by the
source idea: replace exactly one fused-compare or materialized-condition
helper-oracle row with route-native evidence while preserving prepared fallback,
branch-policy, wrapper, and expected-string authority.

Goal: make one named helper-oracle row prove Route 7-native evidence for the
selected comparison or condition fact without weakening the existing prepared
oracle contract.

## Core Rule

Replace one helper-oracle row only. Do not migrate branch policy, printer/debug
formatting, target-wrapper behavior, final assembler behavior, Route 8
return-chain ownership, or generic route-index facades.

## Read First

- `ideas/open/216_route7_comparison_oracle_row.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir.hpp`

## Current Targets

- One selected fused-compare or materialized-condition helper-oracle row in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Route 7 comparison-condition index/query/reference evidence in
  `src/backend/bir/bir.cpp` and `src/backend/bir/bir.hpp`.
- Branch-control coverage that already protects positive, absent-route,
  invalid-reference, duplicate/conflict, mismatch, unfused fallback, and
  prepared fallback behavior.
- Machine-printer and branch-control expected strings that must stay
  byte-stable.

## Non-Goals

- Do not replace more than one helper-oracle row.
- Do not rename helpers, rewrite expected strings, refresh baselines, or
  downgrade supported paths to unsupported paths.
- Do not move branch targets, branch suffix selection, fused legality, hazards,
  emitted-register state, final instruction order, final assembler rows, or
  branch-policy oracle behavior into Route 7.
- Do not contract prepared printer/debug output, target wrappers, public
  fallback, or the prepared lookup aggregate.
- Do not touch Route 8 return-chain work or introduce a generic route-index
  abstraction.

## Working Model

Route 7 can provide route-native evidence for selected comparison-condition
facts: comparison instruction records, comparison operand records, branch
condition records, fused-compare operand producer facts, and materialized
condition records. The selected helper-oracle row may cite Route 7 as the
evidence source only when the current positive and fail-closed/fallback
semantics stay equivalent to the prepared oracle. Prepared/AArch64 behavior
remains the authority for branch policy, emitted output, wrappers, and expected
strings.

## Execution Rules

- Start by naming exactly one helper-oracle row and whether it is fused-compare
  or materialized-condition.
- Preserve the current prepared oracle status for positive, absent-route,
  invalid-reference, duplicate/conflict, mismatch, unfused fallback, and
  prepared fallback cases.
- Use route-native evidence for the selected row; do not claim progress through
  helper renames, expectation rewrites, or classification-only changes.
- Keep branch-control and machine-printer expected strings unchanged.
- For code-changing steps, run a fresh build or targeted CTest proof chosen by
  the supervisor.
- Escalate to reviewer scrutiny if the diff changes expected strings,
  branch-policy output, wrappers, final assembler rows, helper names, supported
  status, or more than one helper-oracle row.

## Steps

### Step 1: Pin the Helper-Oracle Row

Goal: identify the exact helper-oracle row to replace and the full proof matrix
without changing implementation.

Primary target: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Actions:

- Inspect existing Route 7 fused-compare and materialized-condition
  helper-oracle assertions.
- Choose one named row and record whether it is fused-compare or
  materialized-condition.
- Map the selected row to its existing prepared helper, route-native Route 7
  query/reference evidence, and prepared fallback path.
- Map positive, absent-route, invalid-reference, duplicate/conflict, mismatch,
  unfused fallback, prepared fallback, wrapper stability, and expected-string
  proof points into `todo.md`.
- Identify the minimal backend test subset the supervisor should delegate for
  the implementation packet.
- Do not edit implementation while the selected row or proof surface is
  ambiguous.

Completion check:

- `todo.md` names one helper-oracle row, its prepared fallback, the
  route-native evidence source, expected-string owners, wrapper proof, and the
  minimal proof subset.

### Step 2: Add Route-Native Evidence for the Row

Goal: make the selected helper-oracle row assert Route 7-native evidence while
preserving the prepared oracle result.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir.hpp`

Actions:

- Reuse existing Route 7 comparison-condition index/query/reference helpers
  where they already express the selected row.
- Add only the smallest route-native evidence surface needed if the selected
  row cannot be expressed by existing helpers.
- Keep the prepared helper oracle assertion beside the route-native assertion
  unless removing it is explicitly proven unnecessary by the selected row's
  contract.
- Treat absent, invalid, duplicate/conflict, and mismatch Route 7 references as
  fail-closed or prepared fallback according to the current row behavior.
- Avoid branch-policy, printer/debug, wrapper, final assembler, Route 8, or
  generic route-index changes.

Completion check:

- The selected row proves route-native evidence in the positive case and still
  observes the same prepared oracle status for all required fallback cases.

### Step 3: Prove Negative and Fallback Semantics

Goal: prove the selected row is not testcase-shaped and keeps current fallback
behavior authoritative.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`

Actions:

- Cover absent Route 7 evidence with the same fail-closed or fallback status.
- Cover invalid references and stale owners with the current rejection status.
- Cover duplicate or conflicting provenance rows without accepting ambiguous
  route evidence.
- Cover route/prepared mismatch without changing prepared oracle behavior.
- Cover unfused and prepared fallback paths for the selected row family.
- Confirm branch policy, fused legality, suffixes, branch targets, and final
  assembler behavior are not altered.

Completion check:

- The targeted proof covers positive, absent, invalid, duplicate/conflict,
  mismatch, unfused fallback, and prepared fallback behavior without weakening
  expected strings or supported-path contracts.

### Step 4: Prove Expected-String and Wrapper Stability

Goal: show the helper-oracle replacement does not imply printer/debug,
branch-control, wrapper, or final-output contraction.

Primary targets:

- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- Any wrapper proof target named in `todo.md` during Step 1.

Actions:

- Run the delegated branch-control proof and confirm expected strings remain
  byte-stable.
- Run the delegated machine-printer proof for the selected fused or
  materialized-condition family.
- Run the delegated wrapper proof if Step 1 identifies an existing wrapper row
  that could be mistaken for replacement scope.
- Treat expected-string edits, helper renames, baseline refreshes, unsupported
  downgrades, or weaker contracts as blockers unless the supervisor explicitly
  approves them.

Completion check:

- Branch-control, machine-printer, and any selected wrapper output remain
  stable under the one-row route-native helper-oracle replacement.

### Step 5: Acceptance Handoff

Goal: leave the slice ready for supervisor acceptance or reviewer scrutiny.

Actions:

- Update `todo.md` with the selected row, implementation summary, proof
  commands, and results.
- Confirm the source idea remained unchanged.
- Confirm implementation changes are limited to the selected helper-oracle row,
  the minimal Route 7 evidence surface, and targeted tests.
- Flag branch-policy migration, expected-string rewrites, helper renames,
  wrapper contraction, final assembler changes, Route 8 work, generic
  route-index work, or additional helper-oracle rows as blockers.

Completion check:

- The slice has fresh targeted proof, no testcase-overfit signs, no
  expectation weakening, and no branch-policy, printer/debug, wrapper,
  final-assembler, Route 8, or generic route-index migration.
