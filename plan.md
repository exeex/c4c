# Closed Idea Metadata Cleanup Runbook

Status: Active
Source Idea: ideas/open/419_closed_idea_metadata_cleanup_after_prepared_contracts.md

## Purpose

Activate the metadata-only cleanup route for stale lifecycle signals left after
the prepared-contract closure round.

## Goal

Make closed prepared-contract idea files 413 through 418 internally consistent
with their archive location and closure notes.

## Core Rule

Change only lifecycle metadata and short clarification notes. Do not change
technical scope, proof claims, implementation code, tests, or closure history.

## Read First

- ideas/open/419_closed_idea_metadata_cleanup_after_prepared_contracts.md
- ideas/closed/413_*.md through ideas/closed/418_*.md
- AGENTS.md lifecycle rules for archive metadata and commit discipline

## Current Scope

- Closed prepared-contract idea files numbered 413 through 418.
- Stale in-file status headers that disagree with archive location.
- Outdated self-references that still point at `ideas/open/...`.
- Missing or unclear closure discoverability when a short metadata note is
  enough to make the closed state obvious.

## Non-Goals

- Implementation code changes.
- Test, expectation, unsupported-marker, allowlist, runtime-comparison, or
  proof-log changes.
- Reopening, reordering, or reinterpreting prepared-contract ideas.
- Rewriting historical closure notes or regression evidence.
- Broad formatting churn across unrelated idea files.

## Working Model

- The path under `ideas/closed/` is the primary lifecycle state.
- In-file metadata should not contradict the archive path.
- Closure notes and proof evidence are historical records and must be
  preserved.
- Metadata cleanup is docs/lifecycle-only and should be verified with
  `git diff --check`, not RV64 gcc_torture or default CTest.

## Execution Rules

- Inspect all closed prepared-contract idea files 413 through 418 before
  editing so the cleanup is complete and consistent.
- Keep edits minimal and mechanical: status headers, stale self-references, and
  short closure-discoverability notes only.
- Do not touch implementation files, tests, root-level logs, review artifacts,
  or unrelated ideas.
- If a closed idea needs a substantive technical correction, stop and record
  the ambiguity in `todo.md` instead of rewriting history.
- Prove the lifecycle-only slice with `git diff --check`.

## Ordered Steps

### Step 1: Audit Closed Prepared-Contract Metadata

Goal: Identify every stale lifecycle metadata issue in closed idea files 413
through 418.

Primary target: `ideas/closed/413_*.md` through `ideas/closed/418_*.md`.

Actions:

- Read the six closed prepared-contract idea files.
- Record which files still contain `Status: Open`, stale
  `ideas/open/...` self-references, or unclear closure discoverability.
- Confirm that the needed edits are metadata-only and do not alter technical
  claims.
- Record the exact edit list in `todo.md` before changing archive files.

Completion check:

- `todo.md` names each affected file and the metadata-only edits needed, or
  records that no archive edit is required.

### Step 2: Apply Minimal Metadata Corrections

Goal: Bring the affected closed idea metadata into agreement with archive
state.

Primary target: Only the affected files from Step 1.

Actions:

- Change stale `Status: Open` headers to closed-state metadata appropriate for
  the existing file style.
- Correct stale `ideas/open/...` self-references when they misrepresent the
  current archive location.
- Add only short closure-discoverability notes if the existing closure state is
  otherwise hard to find.
- Preserve existing closure notes, proof claims, and historical intent.

Completion check:

- The affected files no longer contain stale open-state metadata, and the diff
  shows no technical or proof-claim changes.

### Step 3: Validate Metadata-Only Cleanup

Goal: Prove the cleanup is mechanically clean and ready for supervisor commit.

Primary target: Lifecycle metadata diff.

Actions:

- Run `git diff --check`.
- Inspect `git diff --stat` and the relevant file diff to confirm only intended
  closed idea metadata changed.
- Record validation results and any remaining caveats in `todo.md`.

Completion check:

- `git diff --check` passes, `todo.md` records proof, and no reviewer reject
  signal from the source idea applies.
