# Route 5 Edge/Join Oracle Printer Row Runbook

Status: Active
Source Idea: ideas/open/212_route5_edge_join_oracle_printer_row.md

## Purpose

Activate one narrow Route 5 follow-up that replaces exactly one edge/join
helper-oracle or prepared-printer row while preserving prepared edge, join,
move-bundle, branch, parallel-copy, wrapper, and expected-string authority.

## Goal

Select one named Route 5 edge/join diagnostic row and make route-native
evidence reproduce the prepared success row only when it agrees with prepared
semantics, with prepared fallback for no-source, memory-source, unsupported
move, duplicate/conflict, absent/mismatch, and wrapper-observed paths.

## Core Rule

Route 5 may provide evidence for the selected diagnostic row only. Prepared
data remains authoritative for unsupported moves, branch and parallel-copy
policy, move-bundle behavior, edge publication mechanics, source-producer
policy, wrapper output, and expected strings.

## Read First

- `ideas/open/212_route5_edge_join_oracle_printer_row.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- Route 5 edge/join helper-oracle and prepared-printer code
- Tests covering joined branches, edge publication, move bundles, prepared
  printer rows, and target wrapper output

## Current Targets

- One named edge/join helper-oracle row or prepared-printer row covering join
  transfer, parallel copy, move bundle, edge publication, or source producer.
- Route-native evidence for the selected row.
- Prepared fallback for no-source, memory-source, unsupported move,
  duplicate/conflict, absent/mismatch, branch/parallel-copy sanity, and wrapper
  output stability.

## Non-Goals

- Do not migrate whole edge-publication, source-producer, move-bundle,
  joined-branch, wrapper, or printer families.
- Do not move branch policy, parallel-copy scheduling, public fallback,
  prepared API ownership, wrapper behavior, or emitted output into Route 5.
- Do not weaken supported-path tests, refresh baselines, rename helpers, or
  rewrite expectations as proof of progress.
- Do not claim aggregate Route 5 migration from one selected diagnostic row.

## Working Model

- The selected row is a diagnostic proof surface, not a new owner for edge,
  join, move-bundle, branch, parallel-copy, or wrapper policy.
- Route 5 evidence is usable only when it identifies the same fact that
  prepared data would report for the selected row.
- Missing, invalid, duplicate/conflicting, unsupported, and mismatched Route 5
  facts must preserve existing prepared diagnostics.
- Adjacent joined-branch, prepared-printer, and wrapper output spelling remain
  compatibility surfaces.

## Execution Rules

- Keep every implementation packet tied to one named diagnostic row.
- Record the selected row, current prepared text, fallback dimensions, and
  narrow proof command in `todo.md` before changing implementation files.
- Add Route 5 evidence behind prepared fallback; do not bypass prepared edge,
  join, move-bundle, branch, parallel-copy, or wrapper authority.
- Treat expectation rewrites, unsupported downgrades, helper renames,
  baseline refreshes, and named-case shortcuts as non-progress.
- For code-changing packets, run fresh build or compile proof plus the narrow
  Route 5 edge/join test subset selected by the supervisor.
- Escalate to wrapper or broader backend validation if shared join helpers,
  output strings, target wrappers, or branch/parallel-copy behavior are
  touched.

## Steps

### Step 1: Name the Row and Baseline Prepared Output

Goal: identify the exact edge/join helper-oracle or prepared-printer row and
record its prepared diagnostic behavior before implementation changes.

Primary targets:
- Route 5 edge/join helper-oracle rows
- Prepared-printer rows for join transfer, parallel copy, move bundle, edge
  publication, or source producer
- Joined-branch, edge-publication, and wrapper expected-string tests

Actions:
- Inspect edge/join diagnostic rows and select one exact row to replace.
- Confirm the row currently comes from prepared edge/join, move-bundle,
  publication, or source-producer data.
- Record the selected row text or row class, prepared baseline behavior,
  fallback dimensions, and narrow proof command in `todo.md`.
- Identify existing positive, no-source, memory-source, unsupported move,
  duplicate/conflict, absent/mismatch, branch/parallel-copy, printer/debug, and
  wrapper coverage for that row.

Completion check:
- `todo.md` names one selected row, proof command, existing coverage, and
  missing proof cases without implementation changes.

### Step 2: Add Route-Native Evidence for the Row

Goal: let Route 5 provide the selected diagnostic row only when it agrees with
prepared semantics.

Primary targets:
- The selected edge/join helper-oracle or prepared-printer row
- Route 5 edge/join evidence lookup helpers
- Prepared fallback paths used by the selected row

Actions:
- Reuse or add a narrow helper that reads Route 5 evidence for the selected
  row.
- Require agreement with the prepared edge, join source, destination,
  publication, move, or row-specific identity fields.
- Preserve prepared diagnostics for no Route 5 fact, memory-source-only
  evidence, unsupported moves, invalid references, duplicate/conflicting rows,
  absent facts, and route/prepared mismatch.
- Keep branch policy, parallel-copy scheduling, move-bundle mechanics, wrapper
  behavior, emitted output, and public fallback ownership unchanged.

Completion check:
- The selected row can use Route 5 evidence under agreement, and every failed
  agreement path returns the existing prepared diagnostic row.

### Step 3: Prove Fail-Closed Edge/Join Diagnostics

Goal: make the positive and negative Route 5 contract observable for the
selected row.

Primary targets:
- Tests for the selected diagnostic row
- Tests for no-source, memory-source, unsupported move, duplicate/conflict,
  absent, invalid, and mismatch cases
- Tests for branch/parallel-copy sanity when the selected row is adjacent to
  those paths

Actions:
- Cover route/prepared agreement for the selected row.
- Cover no Route 5 fact and absent evidence.
- Cover memory-source and unsupported move fallback when applicable.
- Cover invalid references and duplicate or conflicting Route 5 rows when the
  selected path can observe them.
- Cover route/prepared disagreement and prove the prepared row is retained.
- Prove branch and parallel-copy policy remain prepared or target-owned.

Completion check:
- Narrow tests prove positive, absent, invalid, duplicate/conflict when
  applicable, mismatch, unsupported or memory fallback where applicable, and
  branch/parallel-copy sanity without weakening expected strings or
  supported-path contracts.

### Step 4: Preserve Printer, Wrapper, and Joined-Branch Strings

Goal: prove the row replacement did not alter byte-stable output surfaces or
adjacent wrapper behavior.

Primary targets:
- Prepared-printer expected-string tests
- Joined-branch expected-string tests
- x86/riscv wrapper tests selected by the supervisor when relevant

Actions:
- Run the selected printer/debug expected-string proof for the named row.
- Run joined-branch and wrapper no-change checks if shared edge/join helpers,
  target wrappers, or output surfaces were touched.
- Do not update expected strings unless a separate approved semantic change
  requires it.

Completion check:
- Proof logs show the selected row and adjacent joined-branch, printer/debug,
  and wrapper strings remain byte-stable, with wrapper no-change coverage
  where applicable.

### Step 5: Acceptance Review

Goal: prepare the slice for supervisor review without expanding Route 5 scope.

Actions:
- Compare the final diff against the source idea reviewer reject signals.
- Verify no broad edge-publication, source-producer, move-bundle,
  joined-branch, wrapper, or printer-family migration slipped in.
- Verify route/prepared agreement is semantic and fail-closed, not
  testcase-shaped row matching.
- Keep routine progress and proof notes in `todo.md`; do not edit the source
  idea unless durable intent changes.

Completion check:
- The slice satisfies the source idea acceptance criteria and is ready for
  supervisor-side validation and commit.
