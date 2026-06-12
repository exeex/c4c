# Route 4 Block-Entry Publication Printer/Debug Row Runbook

Status: Active
Source Idea: ideas/open/210_route4_block_entry_publication_printer_debug_row.md

## Purpose

Activate one narrow Route 4 diagnostic follow-up that replaces exactly one
block-entry publication printer/debug row with route-native evidence while
keeping prepared publication output and fallback authority intact.

## Goal

Select one named block-entry publication row and make route-native evidence
reproduce its current prepared semantics byte-for-byte, including fail-closed
behavior for absent, invalid, duplicate, ambiguous, and mismatched facts.

## Core Rule

Route 4 may provide evidence for the selected row only. Prepared printer/debug
output remains authoritative for row text, fallback behavior, publication
mechanics, wrapper compatibility, move/home/storage policy, emitted output, and
public prepared surfaces.

## Read First

- `ideas/open/210_route4_block_entry_publication_printer_debug_row.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/mir/backend_x86_publication_plan_reuse_test.cpp`

## Current Targets

- One exact block-entry publication row from prepared value-location or
  publication printer/debug output.
- Route 4 block-entry publication identity evidence for that same row.
- Prepared fallback and diagnostics for missing, wrong-reference, duplicate,
  ambiguous, and mismatched Route 4 facts.
- x86/riscv wrapper no-change proof when shared publication helpers are touched.

## Non-Goals

- Do not migrate wrapper families, CLI dump sections, broad printer/debug
  surfaces, emitted strings, publication mechanics, or target output policy.
- Do not remove public prepared fallback, delete prepared APIs, contract
  `PreparedBirModule`, or claim aggregate migration.
- Do not move move/home/storage policy, block-order emission, immediate payload
  spelling, wrapper formatting, or final output policy into Route 4.
- Do not weaken supported-path tests, refresh baselines, rename helpers, or
  change expected strings as proof of progress.

## Working Model

- The selected row is a diagnostic/oracle replacement prerequisite, not a
  production publication migration.
- Route-native evidence is acceptable only when it agrees with the current
  prepared row semantics and row identity.
- Missing, stale, wrong-reference, duplicate, ambiguous, and mismatched Route 4
  facts must preserve the existing prepared row or absence status.
- Prepared printer/debug remains the compatibility boundary until every
  acceptance case is observable.

## Execution Rules

- Keep every implementation packet tied to the one selected row.
- Record the selected row and exact proof command in `todo.md` when execution
  begins.
- Add route-native evidence only behind agreement and fail-closed checks; do
  not bypass prepared diagnostics.
- Treat expectation rewrites, unsupported downgrades, helper renames, baseline
  refreshes, and named-case shortcuts as non-progress.
- For code-changing packets, run fresh build or compile proof plus the narrow
  Route 4/prepared-printer test subset selected by the supervisor.
- Escalate to wrapper or broader backend validation if shared publication
  helpers, target wrappers, CLI dump output, or expected strings are touched.

## Steps

### Step 1: Name the Row and Baseline Prepared Output

Goal: identify the exact block-entry publication printer/debug row and prove
its current prepared behavior before changing implementation files.

Primary targets:
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/CMakeLists.txt`

Actions:
- Inspect prepared block-entry publication printer output and select one exact
  row string or row class to replace.
- Confirm whether the row currently comes from value-location output,
  publication output, or a route/debug oracle.
- Record the selected row, expected row text, and narrow proof command in
  `todo.md` when execution begins.
- Identify existing positive, absent, invalid, duplicate/conflict, mismatch,
  fallback, and wrapper stability coverage for that row.

Completion check:
- `todo.md` names the selected row, proof command, and any missing proof cases
  without implementation changes.

### Step 2: Add Route-Native Evidence for the Selected Row

Goal: make the selected row reproducible from Route 4 evidence only when it
matches prepared row semantics.

Primary targets:
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- Route 4 block-entry publication identity helpers used by nearby tests

Actions:
- Reuse or add a narrow helper that reads Route 4 block-entry publication
  identity for the selected row.
- Require agreement with prepared successor, destination value, value-home,
  source publication, register/storage details, and any row fields currently
  printed.
- Preserve prepared fallback for missing Route 4 evidence and for every failed
  agreement check.
- Keep prepared lookup construction, publication mechanics, row spelling, and
  wrapper behavior unchanged.

Completion check:
- The selected row can be attributed to Route 4 evidence under agreement, and
  absent or invalid route data still produces the existing prepared behavior.

### Step 3: Prove Fail-Closed Diagnostics

Goal: make the negative and fallback contract observable for the selected row.

Primary targets:
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/CMakeLists.txt`

Actions:
- Cover the positive route/prepared agreement case.
- Cover missing Route 4 publication evidence.
- Cover wrong successor or wrong destination references.
- Cover duplicate, ambiguous, or conflicting rows when the selected row can
  observe them.
- Cover route/prepared mismatch and prepared fallback behavior.

Completion check:
- Narrow tests prove positive, absent, invalid, duplicate/conflict when
  applicable, mismatch, and fallback behavior without expected-string
  weakening.

### Step 4: Preserve Row Text and Wrapper Stability

Goal: prove the diagnostic replacement did not alter byte-stable output
surfaces outside the selected attribution path.

Primary targets:
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/mir/backend_x86_publication_plan_reuse_test.cpp`
- Any riscv publication wrapper test selected by the supervisor

Actions:
- Run the selected prepared-printer or dump expected-string proof for the row.
- Run x86/riscv wrapper no-change checks if shared publication helpers or
  lookup surfaces were touched.
- Do not update expected strings unless a separate approved semantic change
  requires it.

Completion check:
- Proof logs show the selected row text is byte-stable and wrapper output did
  not change.

### Step 5: Acceptance Review

Goal: prepare the slice for supervisor review without expanding the route.

Actions:
- Compare the final diff against the source idea reviewer reject signals.
- Verify no wrapper-family, CLI dump, broad printer/debug, prepared API,
  target-policy, or aggregate migration slipped in.
- Keep routine progress and proof notes in `todo.md`; do not edit the source
  idea unless durable intent changes.

Completion check:
- The slice satisfies the source idea acceptance criteria and is ready for
  supervisor-side validation and commit.
