# Phase F3 Route 4/5 Edge Publication Parity Blocker Map

Status: Active
Source Idea: ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md

## Purpose

Turn idea 251 into an execution runbook for proving, or explicitly blocking,
one narrow Route 4/5 edge-publication identity boundary before prepared
`edge_publications` helpers can become compatibility mirrors.

## Goal

Decide whether one shared route/BIR edge-publication identity has enough x86
and riscv evidence to support a later bounded adapter, while preserving
prepared lookup/status compatibility and target-owned emission policy.

## Core Rule

Do not treat prepared edge-publication lookup results as migrated semantic
authority unless the selected Route 4/5 route/BIR fact is proven for the
relevant target evidence and the prepared answer only mirrors that fact or
fails closed.

## Read First

- `ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md`
- Prepared lookup and status surfaces for edge publication helpers
- Route 4/5 edge/source publication records and consumers
- Existing x86 dispatch/status/module-output tests and riscv prepared
  publication/status tests

## Current Targets / Scope

- One selected edge/source publication identity fact from route/BIR publication
  to prepared edge-publication lookup agreement.
- x86 evidence: dispatch/status/module-output evidence for the selected fact,
  or the exact consumer gap that blocks parity.
- riscv evidence: emission/status evidence checked against the same semantic
  fact.
- Fail-closed cases for duplicate, mismatch, unsupported, prepared-only,
  fallback, and policy-sensitive publication rows.

## Non-Goals

- Do not delete, privatize, rename, or bypass public prepared
  `edge_publications` helpers.
- Do not move move selection, register choice, carrier/helper selection,
  layout, instruction spelling, or target output formatting into BIR.
- Do not rewrite riscv instruction output, x86 dispatch/status strings,
  helper/oracle names, fallback publication rows, or module output rows.
- Do not perform broad publication, edge lowering, or emission-policy rewrites
  outside the selected edge-publication identity.

## Working Model

- Route/BIR owns only the selected semantic edge-publication identity if target
  evidence proves it.
- Prepared edge-publication lookup/status surfaces remain public compatibility
  authority until an adapter can prove agreement and fail closed on
  disagreement.
- Target emission policy remains separate from route/BIR semantic identity.
- A blocked row is a valid result when it names the missing consumer, evidence,
  or fail-closed proof precisely.

## Execution Rules

- Prefer analysis packets first; do not implement an adapter in this plan
  unless a later lifecycle step explicitly creates an implementation idea.
- Keep routine inventory, matrix, and blocker notes in `todo.md`.
- Preserve source idea 251 unless durable intent truly changes.
- Treat testcase-shaped matching, expectation weakening, helper renames,
  output rewrites, or status-string rewrites as route drift.
- For any code-changing packet that a future plan may derive, require build
  proof plus focused x86/riscv backend tests covering the selected fact and
  compatibility rows.

## Step 1: Inventory Edge-Publication Surfaces and Consumers

Goal: identify the exact prepared, route/BIR, x86, and riscv surfaces involved
in Route 4/5 edge-publication identity.

Primary target: lookup helpers, status rows, route/BIR edge/source publication
records, and backend consumers around prepared edge publication handling.

Actions:

- Inventory public prepared `edge_publications` helpers and observable status
  rows.
- Locate Route 4/5 edge/source publication records and any helper accessors.
- Identify x86 dispatch, status, module-output, and prepared-publication
  consumers or absence of consumers for edge-publication identity.
- Identify riscv emission/status consumers tied to the same fact family.
- Record helper/oracle names, fallback publication rows, unsupported rows,
  exact instruction text, and target-policy-sensitive output that must stay
  compatibility-owned.

Completion check:

- `todo.md` lists each relevant surface, consumer bucket, and observed gap
  without proposing implementation.

## Step 2: Select One Candidate Edge-Publication Identity Fact

Goal: choose the narrow edge/source publication fact to test for parity or
block explicitly.

Primary target: the smallest Route 4/5 edge-publication relation that can map
to prepared lookup agreement.

Actions:

- Pick one edge/source publication identity fact that has the best available
  x86 and riscv evidence.
- Define what agreement means between route/BIR publication and prepared
  edge-publication lookup answers.
- Separate semantic identity from move selection, carrier/helper selection,
  register choice, layout, instruction spelling, and output formatting.
- Name rows that remain target policy or compatibility, not semantic identity.

Completion check:

- `todo.md` states the selected fact, agreement rule, excluded policy surfaces,
  and why broader publication parity is outside this plan.

## Step 3: Prove or Block x86 Evidence

Goal: determine whether x86 has concrete dispatch/status/module-output
evidence for the selected edge-publication identity or a named blocker.

Primary target: x86 prepared publication lookup, dispatch/status, module
output, route-debug, and backend emission consumers relevant to the selected
fact.

Actions:

- Trace x86 use of prepared edge-publication lookup/status data for the
  selected fact.
- Check whether x86 consumes the corresponding Route 4/5 fact directly,
  indirectly, or not at all.
- Classify exact x86 dispatch/status/module-output rows that must remain
  compatibility-owned.
- Record the blocker if x86 evidence is missing or indirect.

Completion check:

- `todo.md` records x86 as proven, blocked, or non-applicable for the selected
  fact with concrete file/function/test evidence.

## Step 4: Recheck Riscv Evidence Against the Same Fact

Goal: align riscv emission/status evidence with the selected edge-publication
identity instead of a nearby target-specific behavior.

Primary target: riscv prepared publication statuses, fallback publication
rows, and output rows that can show or block Route 4/5 evidence.

Actions:

- Trace riscv consumption or publication of the selected Route 4/5 fact.
- Confirm which riscv rows prove identity and which only prove target policy
  or compatibility.
- Preserve exact instruction text, fallback/status names, helper/oracle names,
  carrier/helper selection, and emission-policy output as compatibility or
  target policy.
- Record any riscv mismatch, unsupported, prepared-only, fallback, or
  policy-sensitive blockers.

Completion check:

- `todo.md` records riscv evidence for the same selected fact and flags all
  non-semantic output rows.

## Step 5: Build the Fail-Closed Proof Matrix

Goal: define the cases required before any prepared edge-publication adapter
can claim semantic agreement.

Primary target: duplicate, mismatch, unsupported, prepared-only, fallback, and
policy-sensitive edge-publication rows.

Actions:

- For each fail-closed case, name the prepared compatibility surface that must
  stay observable.
- State the required Route 4/5 agreement or rejection behavior.
- Mark which cases are common and which are target-specific.
- Identify tests or existing evidence that would prove the matrix without
  weakening expectations.

Completion check:

- `todo.md` contains a matrix sufficient for reviewer rejection of expectation
  weakening, named-case shortcuts, or old-failure retention.

## Step 6: Decide Adapter Readiness and Close or Split

Goal: conclude whether idea 251 proves a safe later one-adapter implementation
idea or remains blocked.

Primary target: source-idea acceptance criteria and reviewer reject signals.

Actions:

- Summarize whether one shared Route 4/5 edge-publication identity is proven
  for named x86 and riscv evidence.
- If blocked, name the exact missing consumer, evidence, or fail-closed row.
- If safe, propose a separate narrow implementation idea rather than expanding
  this blocker map.
- Preserve prepared publication lookup/status authority and target emission
  policy boundaries in the closure notes.

Completion check:

- Plan owner can close idea 251 as a completed blocker map, optionally with a
  new bounded follow-up idea, after close-time regression/lifecycle checks.
