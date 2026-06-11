# Cross-target route-view reuse beyond x86 Route 6

Status: Active
Source Idea: ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md

## Purpose

Build a disciplined map of x86 and riscv wrapper boundaries that might reuse
AArch64-proven route views while preserving target-local policy.

## Goal

Produce a cross-target route-view reuse table that separates semantic route
facts from target ABI, frame, register, wrapper, formatting, instruction
selection, and emission policy.

## Core Rule

Do not treat the existing x86 Route 6 `ConsumedPlans` reuse as broad
cross-target readiness. Every candidate boundary needs an explicit route view,
fallback behavior, retained target policy, and proof route.

## Read First

- `ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md`
- Existing AArch64 route-view producers and selected consumers for Route 6.
- x86 and riscv wrapper code that consumes prepared route facts or prepared
  compatibility helpers.
- Relevant prior notes for idea 189 only as evidence for the already-proven
  x86 Route 6 boundary.

## Current Scope

- Inventory x86 and riscv wrapper boundaries that consume prepared route facts
  or prepared compatibility helpers.
- Identify which boundaries can plausibly reuse AArch64-proven route views.
- Define fallback behavior for absent, mismatched, ambiguous, or
  policy-sensitive route facts.
- Keep riscv status explicit instead of inferring from x86.
- Produce proof expectations for each candidate.

## Non-Goals

- Do not invent x86-only or riscv-only BIR adapters before an AArch64 semantic
  route view is proven.
- Do not move ABI, frame, register allocation, instruction selection,
  formatting, wrapper, or final emission policy into shared route facts.
- Do not rewrite unrelated target wrappers.
- Do not open draft 155 or delete cross-target prepared compatibility surfaces.
- Do not claim progress through renames, expectation rewrites, or named-case
  shortcuts.

## Working Model

- A reusable route view is semantic and target-neutral.
- Target wrappers may consume a route view only when the route source identity
  and target-local prepared facts agree.
- Policy-sensitive target behavior remains local even when the semantic route
  view is shared.
- Missing or mismatched route facts must fall back to the existing prepared
  path without weakening behavior.

## Execution Rules

- Work in small audit/proof steps; do not combine inventory, classification,
  and implementation into one broad rewrite.
- Prefer `rg` and existing route-view naming to find consumers before editing
  any documentation or code.
- If a candidate boundary needs implementation, record it as a separately
  scoped follow-up instead of silently expanding this audit.
- For any code-changing follow-up, proof must include build evidence plus a
  target-specific narrow test and an adjacent-case sanity check.
- Treat testcase-shaped or target-name-specific matching as route drift.

## Ordered Steps

### Step 1: Inventory target wrapper consumers

Goal: list x86 and riscv wrapper boundaries that read prepared route facts,
prepared compatibility helpers, or selected route-view adapters.

Primary targets:

- x86 target wrapper and lowering surfaces
- riscv target wrapper and lowering surfaces
- shared prepared lookup or compatibility helper declarations used by those
  targets

Actions:

- Inspect existing x86 and riscv route-related consumers.
- Group consumers by route family or semantic fact consumed.
- Mark whether each consumer currently uses prepared facts, a route view, or a
  compatibility adapter.
- Keep x86 and riscv inventories separate.

Completion check:

- The active notes name every discovered wrapper boundary and the lookup/helper
  surface it consumes, with explicit `unknown` markers where ownership cannot
  yet be proven.

### Step 2: Classify reuse candidates and retained target policy

Goal: decide which inventoried boundaries can reuse AArch64-proven route views
  and which must remain target-local or blocked.

Primary targets:

- Route-view producers already proven on AArch64
- x86 and riscv wrapper candidates from Step 1

Actions:

- For each boundary, name the required AArch64-proven route view.
- Record required source-id, route-id, ambiguity, or conflict checks.
- Record fallback behavior for absent, mismatched, ambiguous, or
  policy-sensitive facts.
- Record target-local policy that must not migrate into the route view.
- Mark candidates as ready, blocked, target-local, or unknown.

Completion check:

- The table distinguishes semantic route-view reuse from target-local policy
  migration and gives explicit riscv status for each candidate.

### Step 3: Define proof routes for ready or blocked candidates

Goal: make each candidate actionable without overfitting to one known wrapper
  case.

Primary targets:

- Candidate table from Step 2
- Existing tests or fixtures for nearby same-route wrappers

Actions:

- For ready candidates, name the narrow proof command and adjacent same-feature
  sanity cases that should pass before implementation is accepted.
- For blocked candidates, name the missing AArch64 route view, missing oracle
  coverage, or target-policy ambiguity.
- Identify whether any candidate should become a future implementation idea
  scoped to one wrapper boundary.

Completion check:

- At least one boundary beyond x86 Route 6 has a clear proof route, or the
  notes explain why no additional boundary is ready yet.

### Step 4: Produce the durable cross-target reuse table

Goal: leave a durable audit artifact that future retirement analysis can cite.

Primary targets:

- The chosen audit destination, selected by the executor based on nearby repo
  conventions
- `ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md` only if
  lifecycle policy requires a compact durable update

Actions:

- Write the cross-target table with columns for target, wrapper boundary,
  consumed fact/helper, required route view, fallback, retained target policy,
  readiness, and proof route.
- Include a short summary of why x86 Route 6 evidence does or does not
  generalize.
- Include explicit riscv status.
- Do not rename or delete existing compatibility surfaces.

Completion check:

- The artifact satisfies the source idea acceptance criteria and gives future
  implementation ideas enough information to scope one wrapper boundary at a
  time.

### Step 5: Validate lifecycle and proof sufficiency

Goal: ensure the audit did not turn into implementation drift or testcase
  overfit.

Actions:

- Confirm no implementation files changed unless the supervisor explicitly
  authorized a follow-up implementation packet.
- Confirm no expectation downgrades, unsupported rewrites, or named-case
  shortcuts were used as evidence.
- If docs or audit artifacts changed only, record docs-only validation in
  `todo.md`.
- If any code changed in a later packet, require fresh build proof and the
  supervisor-selected narrow test subset.

Completion check:

- The supervisor can decide whether idea 195 is complete or whether it should
  spawn one or more narrower implementation ideas.
