# Plan Review Metadata Sync Repair

Status: Open
Last Updated: 2026-04-23

## Goal

Keep `todo.md`'s required plan-review metadata synchronized with the hook-managed state so post-commit updates cannot silently remove the `Plan Review Counter` line and break autonomous execution.

## Why This Idea Exists

The `scripts/plan_review_state.py post-commit` path removed the required `Plan Review Counter` line from `todo.md` after a lifecycle repair commit. That breaks the repo's own invariant that the metadata block near the top of `todo.md` must remain present and machine-readable.

## Main Objective

Repair the plan-review state sync so `todo.md` keeps its required metadata lines near the top after hook-backed updates, and the active lifecycle state can remain machine-readable across commits.

## Primary Scope

- `scripts/plan_review_state.py`
- `.plan_review_state.json`
- `todo.md` metadata synchronization

## Constraints

- preserve the existing `todo.md` execution metadata contract
- keep the parser namespace plan out of this repair effort
- do not touch parser implementation files while fixing the lifecycle/tooling bug

## Validation

- reproduce the post-commit sync path that strips the metadata line
- verify `todo.md` retains `Current Step ID`, `Current Step Title`, and `Plan Review Counter`
- confirm the hook state and rendered `todo.md` stay aligned after a post-commit update
