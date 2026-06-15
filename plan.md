# Phase F5 Edge Publications Prepared-Only Fail-Closed Proof

Status: Active
Source Idea: ideas/open/280_phase_f5_edge_publications_prepared_only_fail_closed_proof.md

## Purpose

Prove that one bounded prepared-only `PreparedFunctionLookups::edge_publications`
row is not accepted as Route 5 semantic edge-publication authority.

## Goal

Drive one real edge-publication consumer with a prepared-only public row that
lacks matching Route 5 authority, prove fail-closed behavior, and preserve the
compatible positive path.

## Core Rule

This plan proves one prepared-only edge-publication boundary. Do not demote,
delete, privatize, wrap, retire, or broadly migrate prepared publication APIs
under this runbook.

## Read First

- `ideas/open/280_phase_f5_edge_publications_prepared_only_fail_closed_proof.md`
- Existing backend prepared edge-publication tests and fixtures.
- The real consumer selected in Step 1 before adding proof coverage.

## Current Scope

- Select one natural edge-publication consumer with a real target path.
- Construct one prepared-only `edge_publications` row that has no matching
  Route 5 authority.
- Prove the real consumer rejects that prepared-only row as semantic authority.
- Preserve compatible positive output and exact target behavior.
- Record status/debug evidence that separates prepared-only rejection from
  missing fixture support.

## Non-Goals

- Do not cover every edge-publication family.
- Do not open `edge_publication_source_producers`, metadata, liveness,
  aggregate retirement, draft 155, or broad prepared API hiding.
- Do not weaken fallback, status/debug, route-debug, prepared-printer,
  helper/oracle, wrapper, exact-output, or baseline contracts.
- Do not claim helper-only rejection as backend consumer proof.

## Working Model

The selected row should be visible in `PreparedFunctionLookups::edge_publications`
but should not have matching Route 5 semantic authority. The consumer must fail
closed because semantic authority is absent, not because the fixture is missing,
the edge shape is invalid, or an unrelated mismatch is introduced.

## Execution Rules

- Keep changes narrow to the chosen fixture, test, and `todo.md`.
- Prefer existing fixture helpers and naming patterns over new abstractions.
- Any failure proof must name the prepared-only row and the missing or
  mismatched Route 5 authority.
- Preserve and assert the compatible positive path in the same targeted area.
- Run the supervisor-delegated proof command exactly when provided.
- Treat expectation downgrades, fallback relaxation, route-debug weakening,
  prepared-printer weakening, wrapper weakening, or baseline weakening as route
  drift.

## Step 1: Locate The Real Consumer Fixture

Goal: choose one existing backend edge-publication consumer and fixture that can
exercise `PreparedFunctionLookups::edge_publications` through a real target path.

Actions:

- Inspect existing prepared edge-publication tests and helper fixtures.
- Identify the public prepared `edge_publications` row shape used by the
  consumer.
- Identify the matching Route 5 authority records that normally make the
  compatible path valid.
- Choose exactly one prepared-only row to prove first.

Completion check:

- `todo.md` names the selected fixture, consumer path, prepared-only row shape,
  and the Route 5 authority that will be absent or mismatched.

## Step 2: Add The Prepared-Only Fail-Closed Proof

Goal: add one focused proof that the selected real consumer rejects a
prepared-only `edge_publications` row.

Actions:

- Construct the selected prepared-only public `edge_publications` row.
- Leave the corresponding Route 5 semantic authority absent or intentionally
  mismatched according to Step 1.
- Drive the real consumer path, not a helper-only lookup.
- Assert fail-closed behavior with no compatible output emitted from the
  prepared-only row.
- Assert status/debug evidence that distinguishes the rejection from missing
  fixture support.

Completion check:

- The targeted test proves the prepared-only row is rejected as semantic
  authority, names the missing or mismatched Route 5 authority, and does not
  weaken output or fallback contracts.

## Step 3: Preserve The Compatible Positive Path

Goal: prove the existing compatible edge-publication path still emits the exact
target behavior.

Actions:

- Keep or add assertions for the compatible row that has matching Route 5
  authority.
- Check exact output or target behavior using the existing local convention.
- Keep helper/oracle and prepared-printer visibility intact.

Completion check:

- `todo.md` records the exact compatible behavior preserved and the authority
  facts that make it valid.

## Step 4: Acceptance Proof

Goal: produce the focused build/test proof for this bounded runbook.

Actions:

- Run the supervisor-provided proof command exactly.
- If no command is provided, use the narrow backend prepared edge-publication
  test subset that covers the selected consumer.
- Preserve `test_after.log` as the executor proof log when a test command is
  run.

Completion check:

- The focused proof is green and `todo.md` records the command, result, and
  confirmation that status/debug, fallback, route-debug, prepared-printer,
  helper/oracle, wrapper, exact-output, and baseline behavior were not weakened.

## Step 5: Closeout Notes

Goal: make the bounded coverage result clear without expanding this idea.

Actions:

- Record the proven prepared-only edge-publication row and consumer surface.
- Record any nearby unexamined edge-publication families as remaining scope for
  later ideas, not as hidden completion claims here.
- Confirm no broad prepared API retirement or migration was performed.

Completion check:

- `todo.md` contains enough closeout evidence for the plan owner to decide
  whether idea 280 is complete under its source acceptance criteria.
