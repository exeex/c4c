# Route 3 Prepared-Policy Boundary Hardening Runbook

Status: Active
Source Idea: ideas/open/193_route3_prepared_policy_boundary_hardening.md

## Purpose

Harden the Route 3 memory/source boundary so route-first semantic reads cannot
replace prepared fallback, AArch64 target-addressing policy, or materialization
policy where those concerns still belong outside BIR route facts.

## Goal

Produce a durable Route 3 boundary and proof path that separates target-neutral
memory/source identity from prepared and target-owned policy.

## Core Rule

Route 3 facts may identify semantic memory/source identity, but prepared and
target-specific policy remains authoritative for addressing, frame, relocation,
volatility, materialization, and final memory operands.

## Read First

- ideas/open/193_route3_prepared_policy_boundary_hardening.md
- Relevant Route 3 prepared helper declarations and consumers.
- Recent Route 3 memory/source tests and AArch64 fallback coverage.

## Current Targets And Scope

- Route 3 memory/source identity consumers.
- Prepared Route 3 oracle and fallback helper surfaces.
- AArch64 memory/source lowering paths where route facts and prepared policy
  can disagree, be absent, or become ambiguous.
- Positive and negative proof for route-first reads with prepared fallback.

## Non-Goals

- Do not move AArch64 address-materialization or target-addressing policy into
  BIR route views.
- Do not delete prepared memory/source helpers.
- Do not rework unrelated routes under the Route 3 label.
- Do not treat the idea 190 regression as complete proof for this boundary.
- Do not open draft 155 or contract `PreparedBirModule` fields.

## Working Model

- Route facts can be read first only for target-neutral source identity.
- Prepared fallback remains required when facts are absent, mismatched,
  ambiguous, or policy-sensitive.
- Target wrappers and AArch64 lowering keep ownership of final operand policy.
- Public prepared Route 3 helpers remain migration oracles unless equivalent
  route-native coverage exists.

## Execution Rules

- Prefer semantic boundary repair over named-case matching.
- Keep implementation packets small enough to prove with build plus narrow
  Route 3/AArch64 tests.
- Each code-changing step must record the exact proof command and result in
  `todo.md`.
- Escalate to broader validation if a packet touches shared lowering,
  prepared compatibility helpers, or target wrapper policy.
- Reject expectation rewrites or unsupported downgrades as progress unless the
  supervisor explicitly approves the contract change.

## Ordered Steps

### Step 1: Inventory Route 3 Memory/Source Consumers

Goal: name where Route 3 memory/source identity is consumed and where prepared
fallback or target policy is still required.

Primary targets:

- Route 3 route-view accessors and prepared compatibility helpers.
- AArch64 memory/source lowering consumers.
- Existing tests that cover route-first, fallback, and policy-sensitive paths.

Actions:

- Inspect Route 3 memory/source helper declarations and direct callers.
- Classify each consumer as route-first eligible, fallback-required,
  target-policy-owned, retained oracle, or blocked/unknown.
- Identify public prepared helpers that remain migration oracles.
- Record nearby same-feature cases that should be covered before claiming
  boundary readiness.

Completion check:

- `todo.md` names the consumers, classifications, retained prepared oracles,
  and proof gaps without changing implementation behavior.

### Step 2: Define The Boundary In Code Or Tests

Goal: make the route-first versus prepared-fallback rule explicit at the
consumer boundary.

Primary targets:

- The narrowest Route 3 consumer or helper surface found in Step 1.
- Existing tests for memory/source lowering or route-view fallback.

Actions:

- Add or tighten helper contracts so route facts are used only for
  target-neutral identity.
- Preserve prepared fallback for absent, mismatched, ambiguous, and
  policy-sensitive facts.
- Keep target addressing, frame, relocation, volatility, materialization, and
  final operand decisions outside BIR route facts.
- Avoid broad rewrites or facade-only renames.

Completion check:

- Build proof passes, and the selected consumer has explicit fallback behavior
  without moving target policy into route facts.

### Step 3: Add Positive And Negative Proof

Goal: prove both route-first identity use and prepared fallback behavior.

Primary targets:

- Narrow Route 3/AArch64 tests tied to the selected consumer.
- Any oracle tests that expose prepared fallback or route mismatches.

Actions:

- Add or update one positive route-first case where facts agree.
- Add or update one negative/fallback case that would fail if prepared target
  policy were bypassed.
- Include nearby same-feature coverage when the original failure family has
  obvious adjacent cases.
- Do not weaken supported-path or oracle expectations.

Completion check:

- Narrow proof demonstrates both positive route-first behavior and negative
  fallback behavior, with exact commands recorded in `todo.md`.

### Step 4: Broaden And Document Readiness

Goal: leave future Phase E retirement analysis with a durable Route 3 boundary
statement and proof summary.

Primary targets:

- `todo.md` proof notes for the completed packet.
- Existing documentation or comments only if the boundary is otherwise not
  discoverable from tests and helper contracts.

Actions:

- Summarize the accepted boundary between source identity and prepared/target
  policy.
- Name retained public prepared Route 3 oracle/fallback surfaces.
- Run the supervisor-selected broader validation if shared lowering or helper
  behavior changed.
- Record residual blockers separately from completed boundary hardening.

Completion check:

- The active lifecycle state explains the Route 3 boundary, retained oracles,
  proof commands, and any remaining blockers without claiming draft 155 or
  `PreparedBirModule` readiness.
