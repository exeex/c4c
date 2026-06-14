# Phase F3 Prepared Liveness Authority Blocker Map

Status: Active
Source Idea: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md

## Purpose

Map why `PreparedBirModule::liveness` remains active prepared planning
authority before any demotion, deletion, wrapping, or private-pass-context
proposal is attempted.

## Goal

Produce a consumer and ownership map for `PreparedBirModule::liveness` that
separates identity-only metadata from allocation, storage, helper-planning,
move scheduling, and target-policy authority.

## Core Rule

Do not demote, delete, wrap, rename, or hide `liveness`. This runbook is
analysis-only and may only identify a future bounded one-reader candidate when
the candidate excludes allocation, storage, helper planning, and target
policy.

## Read First

- Source idea:
  `ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md`
- `PreparedBirModule::liveness` declarations, construction, mutation, and
  direct reads.
- Prealloc, regalloc, helper-planning, x86, riscv, prepared printer, and
  status/debug consumers that mention liveness or liveness-derived facts.
- Focused backend tests and expectations for liveness, helper planning,
  target register/storage policy, fallback, and status output.

## Current Scope

In scope:

- Inventory every consumer bucket needed to decide future liveness ownership.
- Classify liveness use as identity-only metadata, allocation, move
  scheduling, storage/home, carrier/helper, target register, target storage,
  output policy, status/debug compatibility, or construction-only.
- Define fail-closed behavior for absent, invalid, stale, mismatch, fallback,
  duplicate/conflict, and policy-sensitive liveness state.
- Decide whether any future implementation candidate can be bounded to one
  identity-only reader with complete fail-closed proof.

Non-goals:

- Code changes.
- Demoting, deleting, wrapping, or privatizing `liveness`.
- Moving register allocation, storage policy, move scheduling, helper
  planning, target register policy, or target storage policy into BIR.
- Rewriting liveness-driven output, status, helper, fallback, or test
  expectations.
- Treating route, invariants, completed phases, notes, or closed metadata work
  as evidence for liveness.

## Working Model

- `liveness` may carry multiple authority roles at once. A future candidate is
  only valid if it names one reader, one semantic fact, the retained prepared
  compatibility surface, and the fail-closed rows that prove no target policy
  moved.
- Allocation, storage, helper-planning, move scheduling, target-register,
  target-storage, and output-policy consumers are blockers, not demotion
  candidates.
- Analysis evidence belongs in `todo.md`; the source idea should only change
  if the durable intent itself changes or lifecycle closure requires a compact
  note.

## Execution Rules

- Keep each step analysis-only unless the supervisor explicitly delegates a
  separate implementation idea.
- Prefer repo searches and nearby tests over inference from field names.
- Record consumer evidence with file/function names and the observed authority
  bucket.
- Treat missing or ambiguous coverage as blocked, not as approval.
- Do not weaken expectations, change unsupported contracts, rename helpers, or
  add compatibility wrappers while executing this blocker map.

## Steps

### Step 1: Liveness Consumer Inventory

Goal: find every current `PreparedBirModule::liveness` producer and consumer
needed for ownership classification.

Primary targets:

- `PreparedBirModule::liveness`
- liveness construction and mutation paths in prepared-module creation
- prealloc/regalloc/helper-planning users
- x86 and riscv backend users
- prepared printer/status/debug users

Actions:

- Search direct field reads, writes, aggregate construction, helper access, and
  test references.
- Group each hit by subsystem and by producer, consumer, or test/assertion
  role.
- Record unresolved indirect users or derived facts in `todo.md` instead of
  changing source intent.

Completion check:

- `todo.md` lists the producer and consumer inventory with enough file/function
  detail for Step 2 classification.

### Step 2: Authority Bucket Classification

Goal: classify each inventory item by the kind of authority it exercises.

Actions:

- Mark each consumer as identity-only metadata, allocation, move scheduling,
  storage/home, carrier/helper, target register, target storage, output
  policy, status/debug compatibility, or construction-only.
- Identify consumers that combine identity with policy or allocation decisions.
- Name the consumers that block demotion, deletion, wrapping, or
  private-pass-context movement.

Completion check:

- `todo.md` separates candidate identity-only rows from blocked policy,
  allocation, storage, helper-planning, and compatibility rows.

### Step 3: Fail-Closed Behavior Matrix

Goal: define the behavior required before any future one-reader liveness
candidate can be trusted.

Actions:

- For each candidate or ambiguous row, describe expected behavior for absent,
  invalid, stale, mismatch, fallback, duplicate/conflict, unsupported, and
  policy-sensitive liveness state.
- Locate existing tests or expectations that already prove those rows.
- Record missing coverage as blockers rather than implementation permission.

Completion check:

- `todo.md` contains a fail-closed matrix and identifies proof gaps for every
  plausible future candidate.

### Step 4: One-Reader Candidate Decision

Goal: decide whether any future implementation packet can be bounded safely.

Actions:

- Evaluate each identity-only row against the fail-closed matrix.
- Reject rows that depend on allocation, storage, helper planning, target
  register/storage policy, output policy, or broad aggregate compatibility.
- If one row remains plausible, name the exact reader, semantic fact, retained
  prepared compatibility surface, and proof set required for a separate source
  idea.

Completion check:

- `todo.md` states either a concrete one-reader candidate contract or that
  `liveness` remains blocked public prepared planning authority.

### Step 5: Final Blocker Map Summary

Goal: finish the analysis-only runbook with a concise readiness decision.

Actions:

- Ensure `todo.md` summarizes consumer buckets, blockers, fail-closed gaps, and
  the final candidate/no-candidate decision.
- Confirm no code, test expectations, output strings, or source intent were
  changed.
- If a separate implementation initiative is justified, ask the plan owner to
  create a new `ideas/open/` record rather than expanding this runbook.

Completion check:

- The source idea acceptance criteria are satisfied: all needed liveness
  consumer buckets are mapped, any future candidate is limited to one
  identity-only reader, and if no such reader exists the field remains blocked
  public prepared planning authority.
