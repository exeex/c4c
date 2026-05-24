# AArch64 Calls Dispatch Bridge Helper Absorption Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md

## Purpose

Absorb bounded helper glue in `calls_dispatch_bridge.*` into the durable
call-boundary adapter ownership without changing call ABI behavior or the
dispatch contract.

## Goal

Make the bridge smaller or clearer by moving, inlining, or consolidating thin
wrappers that only translate already-selected dispatch facts into call-lowering
inputs.

## Core Rule

This is a behavior-preserving helper absorption refactor. Do not change call
ABI behavior, dispatch selection, generated assembly semantics, or test
expectations.

## Read First

- `ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
- Adjacent AArch64 call or dispatch adapter owner files only when a helper is
  moved to an already-durable owner.

## Current Targets

- Bridge-local helpers that only forward, repackage, or lightly adapt
  already-selected dispatch facts.
- Public bridge entry points that dispatch still needs after helper absorption.
- Existing AArch64 call-boundary adapter ownership for translated call-lowering
  inputs.

## Non-Goals

- Do not redesign `dispatch.cpp`.
- Do not perform broad call-family cleanup.
- Do not rename broadly without proving a durable ownership improvement.
- Do not perform phase extraction, naming repair, call ABI redesign, or
  dispatch route redesign.
- Do not move target-specific register, instruction, or call ABI behavior into
  generic layers.
- Do not downgrade tests, convert supported paths to unsupported, or rewrite
  expectations to claim progress.

## Working Model

`calls_dispatch_bridge.*` is allowed to keep the public surface needed by
dispatch. Helpers whose only role is translating selected dispatch facts into
call-lowering inputs should be absorbed into the existing AArch64 call-boundary
adapter owner, either by moving them, inlining them, or consolidating them with
nearby adapter logic.

## Execution Rules

- Keep changes bounded to `calls_dispatch_bridge.*` and the specific receiving
  owner for each absorbed helper.
- Prefer one helper family at a time; keep each packet small enough to review.
- Preserve data flow, diagnostics, error handling, register choices, argument
  placement, return handling, and preserved-value behavior.
- Treat any need for semantic lowering changes as out of scope for this plan.
- After code-changing steps, run a fresh build or compile proof and the
  supervisor-delegated focused AArch64 MIR/codegen subset.
- If a helper cannot be absorbed without changing behavior or broadening
  ownership, leave it in place and document the blocker in `todo.md`.

## Ordered Steps

### Step 1: Map Bridge Helpers and Durable Owners

Goal: identify which bridge helpers are absorption candidates and where each
candidate belongs.

Primary target: `calls_dispatch_bridge.cpp` and
`calls_dispatch_bridge.hpp`.

Concrete actions:

- Inspect helper declarations, definitions, and direct callers.
- Classify helpers as public dispatch entry points, bridge-private adapter
  glue, or behavior-owning call-lowering code.
- For each bridge-private adapter helper, identify the existing durable owner
  that already owns the corresponding call-boundary adapter behavior.
- Record non-absorbable helpers and reasons in `todo.md` instead of expanding
  this runbook.

Completion check:

- `todo.md` names the chosen first helper family, receiving owner, and any
  helpers intentionally left in the bridge.

### Step 2: Absorb One Bounded Helper Family

Goal: move, inline, or consolidate one accepted bridge helper family into its
durable owner.

Primary target: `calls_dispatch_bridge.*` plus only the specific receiving
owner identified in Step 1.

Concrete actions:

- Preserve the public dispatch-facing bridge entry point if dispatch still
  needs it.
- Remove or shrink bridge-private wrapper code whose behavior now lives with
  the durable owner.
- Keep names and interfaces focused on the existing call-boundary adapter
  concept.
- Avoid changing MIR lowering semantics or target-independent code.

Completion check:

- The bridge surface is smaller or clearer.
- Existing call ABI and dispatch behavior are unchanged.
- The delegated build or focused test proof is fresh and recorded in
  `test_after.log` or the supervisor-specified proof artifact.

### Step 3: Repeat Only for Nearby Same-Shape Helpers

Goal: absorb additional helpers only when they share the same ownership shape
and proof surface as the first successful packet.

Primary target: the same bridge and receiving owner area as Step 2.

Concrete actions:

- Reuse the Step 1 classification to choose the next helper only when it is
  clearly same-shape.
- Keep each absorption small; stop before the change becomes a broad call
  cleanup.
- Update `todo.md` with completed helper names and any blockers.

Completion check:

- Each absorbed helper has an explicit owner rationale in `todo.md`.
- No unrelated dispatch, ABI, or expectation churn appears in the diff.
- The delegated proof command remains green after each packet.

### Step 4: Focused Behavior-Preservation Proof

Goal: prove the refactor preserved AArch64 call behavior for the feature
families named by the source idea.

Primary target: validation, not new implementation.

Concrete actions:

- Run the supervisor-selected build proof for the AArch64 backend target.
- Run focused AArch64 MIR/codegen tests covering normal calls, select-chain
  call arguments, call-result source registers, preserved-value
  materialization, and local-load fallback call arguments.
- Escalate to broader validation if multiple packets have landed or the diff
  crosses more than one receiving owner.

Completion check:

- Fresh proof is recorded in canonical lifecycle logs.
- No expectation downgrades, unsupported-test conversions, or hidden semantic
  changes are present.
- Remaining bridge helpers, if any, have documented ownership reasons in
  `todo.md`.
