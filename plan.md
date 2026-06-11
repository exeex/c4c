# Route 5 Current-Block Join-Source Closure Runbook

Status: Active
Source Idea: ideas/open/171_route5_current_block_join_source_migration.md
Activated After: idea 175 closed in commit 727efd960

## Purpose

Confirm whether the retired Route 5 current-block join-source migration can now
close after the separate instruction-dispatch blocker was resolved.

## Goal

Re-run the Route 5 proof route, verify the previous ambient blocker is gone, and
hand the source idea back for closure if the idea acceptance criteria are still
satisfied.

## Core Rule

This runbook is closure validation for the existing Route 5 migration. Do not
expand Route 5 scope, hide additional prepared helpers, or introduce new
semantic lowering work unless validation proves the source idea is still
incomplete.

## Read First

- `ideas/open/171_route5_current_block_join_source_migration.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 5 current-block join-source helper and consumer tests named in prior
  executor proof:
  - `backend_prepared_lookup_helper`
  - `backend_aarch64_current_block_join_routing`
  - `backend_aarch64_instruction_dispatch`

## Current Targets

- Confirm the selected helper/consumer still reads Route 5 edge/join-source
  records where the source idea requires it.
- Re-run the edge publication and join-source oracle coverage.
- Re-run the narrow Route 5 consumer proof that was previously green.
- Re-run the previously blocking instruction-dispatch coverage to determine
  whether close-level confidence is now available.

## Non-Goals

- Do not migrate a second join-source consumer.
- Do not contract prepared current-block join-source helpers in this closure
  run unless a separate plan explicitly authorizes it.
- Do not alter expectation files to manufacture closure.
- Do not move parallel-copy scheduling, register-sharing, move-bundle order, or
  AArch64 edge-copy emission policy into BIR.
- Do not touch unrelated Route 6, Route 7, facade, or return-chain work.

## Working Model

The Route 5 migration work itself is already recorded as exhausted in the source
idea. The only remaining question is whether the external instruction-dispatch
failure that blocked close-level confidence has been resolved. If the matching
proof is green and no stale Route 5 acceptance criterion is contradicted, the
supervisor should route the active state to plan-owner close review.

## Execution Rules

- Treat this as a validation-first plan; implementation is allowed only for a
  newly discovered Route 5 regression.
- Keep any execution progress in `todo.md`.
- If validation exposes a distinct non-Route-5 failure, record the blocker in
  `todo.md` and stop for supervisor routing instead of expanding this plan.
- If validation proves the source idea complete, request plan-owner closure
  rather than editing `ideas/open/171_route5_current_block_join_source_migration.md`
  during execution.

## Ordered Steps

### Step 1: Reconstruct Route 5 Proof Targets

Goal: identify the exact available CTest targets or commands for the Route 5
oracle, selected consumer, and formerly blocking instruction-dispatch coverage.

Primary target: test inventory and prior proof target names.

Actions:

- Inspect the build test list for Route 5-related tests.
- Map the source idea proof route to concrete commands.
- Confirm whether `backend_prepared_lookup_helper`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_aarch64_instruction_dispatch` are still valid test names.

Completion check:

- `todo.md` records the exact proof commands selected by the supervisor or
  executor for Step 2.

### Step 2: Run Route 5 Closure Proof

Goal: prove the selected helper/consumer coverage and the former close blocker
under fresh build/test output.

Primary target: narrow Route 5 proof plus the formerly blocking
instruction-dispatch test.

Actions:

- Run the selected Route 5 helper/oracle proof.
- Run the selected current-block join routing proof.
- Run `backend_aarch64_instruction_dispatch` or the supervisor-approved
  equivalent that covers the former `expected selected f64 global readback to
  feed call ABI move` failure.
- Preserve proof output in `test_after.log` if the executor packet owns
  validation logging.

Completion check:

- Fresh proof shows no Route 5 regression and no recurrence of the former
  instruction-dispatch blocker, or `todo.md` records the exact failing command
  and first failing fact.

### Step 3: Decide Closure Readiness

Goal: determine whether source idea 171 can move to close review.

Primary target: `ideas/open/171_route5_current_block_join_source_migration.md`
acceptance criteria.

Actions:

- Compare proof results against the source idea acceptance criteria.
- Verify that missing predecessor, no-source, and memory-source cases remain
  covered by the selected proof route.
- Verify no additional prepared current-block join-source helper contraction is
  being claimed by this closure runbook.
- If complete, report that the supervisor should ask plan-owner to close the
  idea using the normal close gate.

Completion check:

- `todo.md` states either closure-ready with proof commands, or blocked with the
  smallest unresolved Route 5 or external failure.
