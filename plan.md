# Scalar Compare Publication Runbook

Status: Active
Source Idea: ideas/open/617_scalar_compare_publication.md

## Purpose

Activate the scalar compare publication idea as a narrow prepared-authority repair route.

## Goal

Repair or conclusively reclassify the currently visible scalar compare publication rows without changing branch, select, or RV64 instruction-fragment ownership.

## Core Rule

Treat scalar compare publication as the only owned capability. Do not use this low-count tail idea to justify broad compare lowering, branch rewrites, select publication changes, RV64 consumer changes, expectation changes, unsupported-marker edits, or named-case shortcuts.

## Read First

- `ideas/open/617_scalar_compare_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- Existing prepared compare, branch, select, and publication diagnostics before editing code.

## Current Targets And Scope

- Prepared scalar compare publication authority.
- Diagnostic evidence that separates scalar compare publication from branch publication, select publication, source freshness, and RV64 instruction-fragment ownership.
- All currently visible scalar compare publication rows, if the family is still present.

## Non-Goals

- General compare lowering.
- Branch stack-source or branch terminator work.
- Select publication or select source wiring.
- RV64 instruction-fragment consumer work.
- ABI, runtime, expectation, unsupported marker, allowlist, timeout, or accounting changes.
- Broad rewrites outside the prepared scalar compare publication path.

## Working Model

- First confirm whether scalar compare publication is still the first owner for the target rows after idea `616`.
- If diagnostics show another first owner, preserve that evidence and reclassify rather than forcing an implementation.
- If scalar compare publication remains the first owner, repair the prepared publication path semantically and prove the complete small family when practical.

## Execution Rules

- Keep packet updates in `todo.md`; do not rewrite this runbook for routine progress.
- Preserve source-idea intent; do not edit `ideas/open/617_scalar_compare_publication.md` unless lifecycle state genuinely changes.
- Make diagnostics distinguish branch, select, scalar compare publication, and RV64 consumer ownership.
- Reject testcase-shaped matching or named source-file shortcuts.
- For code-changing steps, run at least the focused backend test that covers the touched publication path, then the supervisor-selected backend or torture subset.

## Step 1: Refresh Scalar Compare Publication Evidence

Goal: establish whether the scalar compare publication family still exists and identify the current first owner for every visible row.

Actions:
- Inspect the current failure map and high-yield follow-up notes for scalar compare publication rows.
- Run or reuse a current diagnostics probe only if it reflects the post-idea-616 tree.
- Separate rows into scalar compare publication, branch/select/source freshness, RV64 instruction-fragment, runtime, and unrelated owners.
- Record the exact target rows and first-owner evidence in `todo.md`.

Completion Check:
- `todo.md` lists the current scalar compare publication target set or explains why no such row remains.
- The next packet is either implementation for confirmed scalar compare publication ownership or lifecycle close/reclassification for exhausted scope.

## Step 2: Locate Prepared Scalar Compare Publication Authority

Goal: map the failing rows to the prepared-layer producer, carrier, and consumer points that should publish scalar compare operands or results.

Actions:
- Inspect the prepared compare/publication helpers and their diagnostic emission sites.
- Identify whether scalar compare values lose publication as sources, destinations, join carriers, or terminator-adjacent values.
- Confirm that branch and select publication boundaries remain separate.
- Add focused diagnostics only if needed to expose semantic ownership; do not add permanent noisy logging.

Completion Check:
- The owned authority gap is localized to concrete prepared-layer code, or the rows are reclassified to a different owner with evidence.

## Step 3: Repair Scalar Compare Publication Semantics

Goal: publish scalar compare values through the prepared authority path without special-casing target files.

Actions:
- Implement the smallest semantic repair in the prepared scalar compare publication path.
- Preserve existing branch, select, and RV64 consumer contracts.
- Add or update focused tests for the scalar compare publication behavior when an existing test surface exists.
- Avoid expectation rewrites as proof of progress.

Completion Check:
- Focused tests for the touched prepared publication path pass.
- Current scalar compare publication rows no longer fail with the same first-owner diagnostic, or each remaining row has a different concrete first owner.

## Step 4: Prove Family Outcome And Classify Residuals

Goal: finish the idea with complete evidence for the small family.

Actions:
- Re-run the full current scalar compare publication row set.
- Record progressed rows and any residual owner classifications.
- Run the supervisor-selected broader backend or torture proof if implementation changed.
- Prepare a close-readiness summary in `todo.md`.

Completion Check:
- Acceptance criteria from `ideas/open/617_scalar_compare_publication.md` are met or the idea is explicitly blocked with evidence.
- No complete-authority scalar compare publication row remains unhandled without a separate owner.
