# Sema Canonical Symbol Template Key Authority Closure Gate Runbook

Status: Active
Source Idea: ideas/open/160_sema_canonical_symbol_template_key_authority.md

## Purpose

Keep the canonical symbol template-key authority idea open until closure has an
accepted regression gate. The implementation runbook is exhausted, but the
source idea cannot be archived while the only full-suite candidate baseline is
explicitly rejected.

## Goal

Produce closure-quality proof for the completed canonical symbol identity route
without accepting unrelated non-canonical-symbol failures as a full-suite
baseline.

## Core Rule

Do not close the source idea unless the close-time regression guard passes with
canonical `test_before.log` and `test_after.log` for the same accepted scope.
Do not accept `test_baseline.new.log` as the full-suite baseline while it still
contains the known non-canonical-symbol failures listed below.

## Read First

- `ideas/open/160_sema_canonical_symbol_template_key_authority.md`
- `todo.md`
- `test_before.log`
- Any supervisor-provided canonical `test_after.log`

## Current Scope

- Closure validation only.
- Lifecycle state for the active source idea.
- No implementation files or tests.
- No proof-log rewriting by the plan owner.

## Non-Goals

- Do not change implementation code.
- Do not change tests or expectations.
- Do not weaken or reinterpret the source idea's acceptance criteria.
- Do not move the source idea to `ideas/closed/` until close acceptance passes.
- Do not accept `test_baseline.new.log` as the full-suite baseline while it
  contains:
  - `frontend_parser_lookup_authority_tests`
  - `cpp_c4_static_assert_if_constexpr_branch_unlocks_later`
  - `cpp_c4_static_assert_multistage_shape_chain`

## Execution Rules

- Treat the previous implementation route as exhausted.
- Treat selected sema metadata validation as useful narrow proof, not as the
  close-time regression guard by itself.
- If the supervisor chooses a narrower accepted close scope, the before and
  after logs must still be canonical and command-matched.
- If a future canonical regression guard passes and the source criteria remain
  satisfied, the plan owner may close the idea and remove `plan.md`/`todo.md`.

## Step 1: Acquire Accepted Closure Regression Gate

Goal: Obtain accepted close-time regression evidence for the completed source
idea.

Actions:
- Use only canonical `test_before.log` and `test_after.log` for the same test
  command.
- Reject `test_baseline.new.log` as a full-suite baseline while the three known
  non-canonical-symbol failures remain.
- Compare the canonical logs with the repo regression guard.
- Preserve the selected sema metadata proof as supporting evidence in
  `todo.md`.

Completion Check:
- The regression guard passes for the accepted close scope.
- The scope and command are clear enough for a plan owner to make the close
  decision without re-deriving the validation context.

## Step 2: Close Or Reassess

Goal: Finish the lifecycle decision once closure proof is acceptable.

Actions:
- Recheck the source idea acceptance criteria.
- If the source criteria are satisfied and the close guard passed, move
  `ideas/open/160_sema_canonical_symbol_template_key_authority.md` to
  `ideas/closed/` and remove `plan.md` and `todo.md`.
- If the guard still cannot be accepted because failures are unrelated
  baseline debt, keep the source idea open and record the exact blocker in
  `todo.md`.
- If new canonical-symbol work is discovered, write a fresh runbook step or a
  separate `ideas/open/` initiative instead of silently expanding this gate.

Completion Check:
- Lifecycle state is canonical: either the source idea is closed with active
  files removed, or the open idea has active `plan.md`/`todo.md` that identify
  the remaining closure blocker.
