# Phase E2 Control-Flow Branch Target Helper Private Pass-Context

Status: Active
Source Idea: ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md

## Purpose

Contract the public prepared helper family around
`find_prepared_control_flow_branch_target_labels(...)` so the selected
agreement-proven branch-target identity read moves behind private pass-context
access.

Goal: remove the duplicate public prepared semantic identity read for this
helper family without changing fallback, policy, oracle, printer/debug,
wrapper, or expected-string behavior.

## Core Rule

Only move the selected BIR structured successor identity read. Prepared remains
authoritative for absent, invalid-id, duplicate/conflict, mismatch,
non-agreement fallback, branch/output policy, edge-copy scheduling,
join-transfer records, wrapper output, printer/debug rows, helper-oracle names
and statuses, and expected strings.

## Read First

- `ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/prepared_printer/control_flow.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets And Scope

- Public helper definitions and wrappers for
  `find_prepared_control_flow_branch_target_labels(...)` and its immediate
  helper family.
- Private pass-context access for the selected BIR function/block graph,
  block-label, and structured terminator successor identity read.
- Existing prepared helper-oracle coverage for positive, absent, invalid-id,
  mismatch, and fallback behavior.
- Production consumers that rely on prepared control-flow branch and handoff
  behavior, including x86 module lowering and joined-branch wrappers.

## Non-Goals

- Do not retire aggregate `PreparedControlFlow`, `PreparedFunctionLookups`, or
  `PreparedBirModule`.
- Do not delete public prepared APIs outside this exact helper-family boundary.
- Do not move branch spelling, edge-copy scheduling, block-order emission, move
  policy, wrapper behavior, route-debug rows, printer/debug rows, diagnostics,
  helper-oracle strings, or expected-string ownership into BIR.
- Do not perform E3, E4, Route 8, draft 155, or E5 work.
- Do not claim progress through facade renames, wrapper moves, construction
  reshuffles, baseline refreshes, unsupported downgrades, timeout masking, or
  expectation rewrites.

## Working Model

- BIR structured control-flow identity is proven only for the selected
  branch-target labels under the agreement gate added by the closed Phase E1
  control-flow helper slice.
- A private pass-context boundary may read the BIR identity facts where all
  migrated consumers can safely use that boundary.
- Prepared fallback remains public until consumer proof shows each current
  production, wrapper, printer/debug, oracle, and expected-string dependency is
  either migrated or intentionally retained.

## Execution Rules

- Keep changes small enough that each step can be proven with a build and a
  narrow test subset chosen by the supervisor.
- Preserve byte-stable output for branch spelling, wrappers, printer/debug
  rows, helper-oracle statuses, and expected strings unless the supervisor
  explicitly approves a separate expectation change.
- Treat unsupported downgrades, helper-oracle weakening, or testcase-shaped
  branch-target matching as route failure.
- When a code-changing step affects shared control-flow or pass-context
  behavior, run at least build proof plus the delegated narrow backend/helper
  subset; escalate to broader validation before close.

## Ordered Steps

### Step 1: Map Branch-Target Helper Consumers And Baseline

Goal: establish the exact current helper-family boundary and proof subset
before editing code.

Primary targets:

- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/prepared_printer/control_flow.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- x86 lowering and joined-branch wrapper consumers found by source search

Actions:

- Inventory every direct public caller of
  `find_prepared_control_flow_branch_target_labels(...)` and immediate wrapper
  use.
- Classify each consumer as selected identity read, retained prepared fallback,
  policy/output behavior, oracle behavior, printer/debug behavior, or expected
  string coverage.
- Identify the narrow build/test command that proves the helper family before
  implementation.
- Record the inventory, retained prepared responsibilities, and proof command
  in `todo.md`.

Completion check:

- `todo.md` names all discovered consumers and the delegated baseline/proof
  command.
- No implementation files are modified in this step.

### Step 2: Introduce Private Pass-Context Branch-Target Identity Read

Goal: add the private boundary for the selected BIR structured successor
identity read without changing public fallback or output behavior.

Primary targets:

- pass-context and prepared lookup implementation files identified in Step 1
- `src/backend/prealloc/control_flow.hpp`

Actions:

- Extract or route only the agreement-proven branch-target label identity read
  through private pass-context access.
- Keep absent, invalid, duplicate/conflict, mismatch, and non-agreement paths
  on the prepared fallback surface.
- Avoid aggregate control-flow API retirement and avoid public facade reshaping
  outside the selected helper family.
- Build and run the delegated narrow helper/backend subset.

Completion check:

- Positive agreement cases still read the same branch-target labels through
  the private pass-context boundary.
- Fallback behavior remains reachable through prepared control-flow authority.
- Build and delegated narrow tests pass, with proof recorded in `test_after.log`
  or the supervisor-delegated artifact.

### Step 3: Migrate Safe Production Consumers And Retain Required Public Surface

Goal: move consumers that only need the selected identity read to the new
private boundary while explicitly retaining public prepared access for the
remaining responsibilities.

Primary targets:

- production lowering and wrapper call sites discovered in Step 1
- prepared printer/debug and helper-oracle surfaces

Actions:

- Migrate safe selected-identity consumers to private pass-context access.
- Leave policy/output, wrapper handoff, printer/debug rows, helper-oracle
  names/statuses, and expected-string behavior on the prepared surface.
- Remove only the duplicate public semantic identity read if every current
  consumer has been migrated or retained for a documented prepared reason.
- Build and run the delegated narrow subset.

Completion check:

- Every Step 1 public consumer is accounted for as migrated or retained.
- No target output, printer/debug row, wrapper output, oracle string, or
  expected string changes are needed.
- Build and delegated narrow tests pass.

### Step 4: Add Focused Agreement And Fallback Proof

Goal: prove the contraction is semantic and not shaped around one testcase.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- adjacent same-feature control-flow helper coverage selected by the executor
  and supervisor

Actions:

- Ensure positive agreement proof covers BIR structured successor identity.
- Ensure absent, invalid-id, duplicate/conflict, and mismatch behavior fails
  closed to prepared fallback.
- Ensure at least one nearby same-feature control-flow helper case remains
  covered.
- Do not downgrade supported-path status or weaken helper-oracle contracts.

Completion check:

- Focused tests demonstrate agreement and each retained fallback mode required
  by the source idea.
- Nearby same-feature coverage is present.
- Build and delegated narrow tests pass.

### Step 5: Broader Validation And Closeout Readiness

Goal: prove the helper-family contraction is ready for lifecycle closure or a
plan-owner close review.

Primary targets:

- `todo.md`
- canonical regression logs selected by the supervisor

Actions:

- Run the supervisor-selected broader backend or full validation checkpoint.
- Confirm output policy, wrappers, printer/debug rows, helper-oracle behavior,
  and expected strings remain byte-stable.
- Confirm no aggregate API retirement, E3/E4/Route 8/draft 155/E5 work, or
  unsupported downgrades entered the slice.
- Summarize completed scope and any leftover non-blocking follow-up in
  `todo.md`.

Completion check:

- Broader validation passes.
- `todo.md` records the proof, consumer accounting, and closeout summary.
- The active plan is ready for plan-owner completion judgment.
