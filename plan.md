# Plan Review Metadata Sync Repair

Status: Active
Source Idea: ideas/open/83_plan_review_metadata_sync_repair.md

## Purpose

Keep `todo.md`'s plan-review metadata in sync with the hook-managed state so the required `Plan Review Counter` line is not stripped by post-commit processing.

## Goal

Repair the plan-review metadata synchronization path and validate that `todo.md` remains machine-readable after hook-backed updates.

## Core Rule

Keep this runbook limited to lifecycle metadata sync and the hook path that rewrites `todo.md`. Do not widen it into parser implementation work.

## Read First

- ideas/open/83_plan_review_metadata_sync_repair.md
- `scripts/plan_review_state.py`
- `todo.md`
- `.plan_review_state.json`

## Scope

- `scripts/plan_review_state.py`
- `.plan_review_state.json`
- `todo.md`

## Non-Goals

- no parser implementation changes
- no expansion into namespace lookup or frontend compiler work
- no broad repo-wide lifecycle rewrite

## Working Model

- treat `todo.md` as the canonical rendered lifecycle state for the active plan
- keep the hook state and the file header aligned
- preserve the required metadata block near the top of `todo.md`

## Execution Rules

- reproduce the metadata-loss path before changing behavior
- keep the fix local to the plan-review sync path
- preserve `Current Step ID`, `Current Step Title`, and `Plan Review Counter`
- validate with the same post-commit path that previously stripped the line

## Validation

- `python scripts/plan_review_state.py post-commit`
- confirm `todo.md` still contains the required metadata lines after the hook path runs

## Step 1: Reproduce The Metadata Sync Failure

Goal: confirm the post-commit sync path can still strip the `Plan Review Counter` line from `todo.md`.

Primary targets:

- `scripts/plan_review_state.py`
- `.plan_review_state.json`
- `todo.md`

Actions:

- inspect the post-commit write path that updates `todo.md`
- reproduce the state where the counter line disappears
- capture the exact metadata mismatch the hook creates

Completion check:

- the failure mode is understood well enough to fix without widening the scope

## Step 2: Repair And Validate Metadata Preservation

Goal: keep the required lifecycle metadata block intact after hook-backed updates.

Primary targets:

- `scripts/plan_review_state.py`
- `.plan_review_state.json`
- `todo.md`

Actions:

- adjust the sync path so required metadata lines remain present
- make sure the rendered `todo.md` stays aligned with the hook state
- validate the same post-commit path after the fix

Completion check:

- `todo.md` retains the required metadata block after the hook path runs, and the active lifecycle state stays machine-readable
