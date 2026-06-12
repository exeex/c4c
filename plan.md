# Route 5 Current-Block Join-Source Semantic Reader Runbook

Status: Active
Source Idea: ideas/open/211_route5_current_block_join_source_semantic_reader.md

## Purpose

Activate one narrow Route 5 follow-up that moves exactly one current-block
join-source semantic reader to route/prepared agreement while keeping prepared
edge, join, value-home, and move-bundle authority intact.

## Goal

Select one named current-block join-source reader and make Route 5 provide the
same source identity only when it agrees with prepared semantics, with
prepared fallback for absent, invalid, duplicate/conflict, and mismatched route
facts.

## Core Rule

Route 5 may validate the selected current-block join-source identity only.
Prepared data remains authoritative for prepared edges, joins, value homes,
move bundles, branch policy, parallel-copy scheduling, execution placement,
final move records, emitted output, wrappers, and expected strings.

## Read First

- `ideas/open/211_route5_current_block_join_source_semantic_reader.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- Route 5 join-source helpers and their current prepared fallbacks
- Tests covering joined-branch behavior and prepared output strings
- Target wrapper tests that could observe joined-branch or prepared output

## Current Targets

- One named current-block join-source semantic reader.
- Route 5 source identity evidence for the same current-block join source.
- Prepared fallback for no-source, invalid source references,
  duplicate/conflicting Route 5 facts, and route/prepared mismatch.
- Joined-branch and prepared output byte-stability proof.

## Non-Goals

- Do not migrate edge-publication, source-producer, move-bundle, or x86
  joined-branch families.
- Do not move branch policy, parallel-copy scheduling, execution-site
  placement, cycle temporaries, source/destination homes, final move records,
  emitted output, or wrapper behavior into Route 5.
- Do not weaken supported-path tests, refresh baselines, rename helpers, or
  rewrite expectations as proof of progress.
- Do not claim aggregate Route 5 migration from one selected reader.

## Working Model

- The selected reader is a semantic source reader, not a movement or output
  policy owner.
- Route 5 evidence is usable only when it identifies the same current-block
  join source that prepared data would use.
- Missing, invalid, duplicate/conflicting, and mismatched Route 5 facts must
  preserve existing prepared behavior.
- Joined-branch and prepared output spelling remain compatibility surfaces.

## Execution Rules

- Keep every implementation packet tied to one named current-block
  join-source reader.
- Record the selected reader, current prepared behavior, and narrow proof
  command in `todo.md` before changing implementation files.
- Add Route 5 agreement checks behind prepared fallback; do not bypass
  prepared join, edge, value-home, or move-bundle authority.
- Treat expectation rewrites, unsupported downgrades, helper renames,
  baseline refreshes, and named-case shortcuts as non-progress.
- For code-changing packets, run fresh build or compile proof plus the narrow
  Route 5/join-source test subset selected by the supervisor.
- Escalate to wrapper or broader backend validation if shared join helpers,
  output strings, or target wrapper behavior are touched.

## Steps

### Step 1: Name the Reader and Baseline Prepared Behavior

Goal: identify the exact current-block join-source semantic reader and record
its existing prepared behavior before implementation changes.

Primary targets:
- Route 5 current-block join-source lookup helpers
- Joined-branch and prepared-output tests
- Target wrapper tests that observe joined-branch output

Actions:
- Inspect current join-source readers and select one exact reader to migrate.
- Confirm the reader currently uses prepared source identity, prepared edge or
  join state, and prepared fallback behavior.
- Record the selected reader, current prepared behavior, missing-source
  behavior, and narrow proof command in `todo.md`.
- Identify existing positive, absent, invalid, duplicate/conflict, mismatch,
  fallback, joined-branch, and wrapper output coverage for that reader.

Completion check:
- `todo.md` names one selected reader, proof command, existing coverage, and
  missing proof cases without implementation changes.

### Step 2: Add Route/Prepared Agreement for the Reader

Goal: allow Route 5 to provide the selected join-source identity only when it
matches prepared semantics.

Primary targets:
- The selected current-block join-source reader
- Route 5 source identity lookup helpers
- Prepared fallback paths used by the selected reader

Actions:
- Reuse or add a narrow helper that reads Route 5 current-block join-source
  identity for the selected reader.
- Require agreement with the prepared current block, join edge, source value,
  and any reader-specific source identity fields.
- Preserve prepared behavior for no Route 5 source, invalid source reference,
  duplicate/conflicting source facts, and route/prepared mismatch.
- Keep move-bundle, branch, parallel-copy, source/destination home,
  execution-site, final move, emitted output, and wrapper policy prepared or
  target-owned.

Completion check:
- The reader can use Route 5 identity under agreement, and every failed
  agreement path returns the existing prepared source behavior.

### Step 3: Prove Fail-Closed Join-Source Behavior

Goal: make the positive and negative Route 5 contract observable for the
selected reader.

Primary targets:
- Tests for the selected join-source reader
- Tests for Route 5 invalid, duplicate/conflict, and mismatch cases
- Tests for prepared fallback when Route 5 evidence is absent

Actions:
- Cover route/prepared agreement for the selected reader.
- Cover no Route 5 source fact.
- Cover invalid source references.
- Cover duplicate or conflicting Route 5 source rows when the selected path
  can observe them.
- Cover route/prepared disagreement and prove the prepared source is retained.
- Prove move-bundle, branch, and parallel-copy policy remain prepared or
  target-owned.

Completion check:
- Narrow tests prove positive, absent, invalid, duplicate/conflict when
  applicable, mismatch, and fallback behavior without weakening expected
  strings or supported-path contracts.

### Step 4: Preserve Joined-Branch and Prepared Output Strings

Goal: prove the semantic reader migration did not alter byte-stable output
surfaces or wrapper behavior.

Primary targets:
- Joined-branch expected-string tests
- Prepared output string tests
- x86/riscv wrapper tests selected by the supervisor when relevant

Actions:
- Run the selected joined-branch and prepared-output expected-string proof.
- Run wrapper no-change checks if shared join helpers, target wrappers, or
  output surfaces were touched.
- Do not update expected strings unless a separate approved semantic change
  requires it.

Completion check:
- Proof logs show joined-branch and prepared output strings remain unchanged,
  with wrapper no-change coverage where applicable.

### Step 5: Acceptance Review

Goal: prepare the slice for supervisor review without expanding Route 5 scope.

Actions:
- Compare the final diff against the source idea reviewer reject signals.
- Verify no edge-publication, source-producer, move-bundle, x86
  joined-branch, wrapper-family, or output-policy migration slipped in.
- Verify route/prepared agreement is semantic and fail-closed, not
  testcase-shaped handling for one named case.
- Keep routine progress and proof notes in `todo.md`; do not edit the source
  idea unless durable intent changes.

Completion check:
- The slice satisfies the source idea acceptance criteria and is ready for
  supervisor-side validation and commit.
