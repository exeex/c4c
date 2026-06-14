# Phase F3 Edge Publication Source-Producer Blocker Map

Status: Active
Source Idea: ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md

## Purpose

Turn idea 252 into an execution runbook for mapping whether prepared
`edge_publication_source_producers` authority has any safe x86/riscv exit path
before a producer-index or adapter implementation idea is allowed.

## Goal

Decide whether one narrow edge-publication source-producer relation has named
x86 and riscv parity or exclusion evidence, while preserving public prepared
lookup authority and compatibility-visible helper/status/output rows.

## Core Rule

Do not treat prepared source-producer lookup authority as migrated semantic
authority unless the selected producer/source relation has explicit x86/riscv
parity or non-applicability evidence and a fail-closed duplicate/conflict
matrix.

## Read First

- `ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md`
- Prepared `edge_publication_source_producers` lookup helpers and observable
  helper/status rows
- AArch64 and prepared helper consumers that keep the public prepared surface
  observable
- x86 and riscv edge-publication source/producer consumers or their absence
- Route 5 edge/source evidence only as supporting context, not as readiness
  proof by itself

## Current Targets / Scope

- One selected source-producer relation attached to prepared edge-publication
  source lookup authority.
- Prepared and AArch64 consumers that force the public prepared helper surface
  to remain compatibility-owned.
- x86 evidence: direct consumption, indirect consumption, explicit
  non-applicability, or a named blocker for the selected relation.
- riscv evidence: direct consumption, indirect consumption, explicit
  non-applicability, or a named blocker for the same relation.
- Fail-closed duplicate, conflict, mismatch, missing, prepared-only, and
  policy-sensitive rows for any later producer/source adapter.

## Non-Goals

- Do not implement a BIR producer index.
- Do not delete, privatize, wrap, rename, or bypass public prepared
  source-producer lookup helpers.
- Do not infer readiness from Route 5 edge/source evidence alone.
- Do not rewrite publication/output baselines, helper/oracle names, fallback
  names, compatibility strings, or prepared status rows.
- Do not perform broad AArch64, x86, riscv, publication, or producer-index
  rewrites outside this blocker map.

## Working Model

- Prepared `edge_publication_source_producers` remains public compatibility
  authority until an adapter can prove the selected relation or reject it
  fail-closed.
- AArch64/prepared helper consumers are compatibility evidence, not migration
  proof.
- Route 5 edge/source evidence can provide context, but it does not by itself
  prove source-producer lookup ownership transfer.
- A blocked, unknown, or explicitly non-applicable bucket is valid only when
  it names the exact consumer, gap, or exclusion evidence.

## Execution Rules

- Prefer analysis packets; this plan is a blocker map, not an implementation
  plan.
- Keep routine inventories, classifications, matrices, and blocker notes in
  `todo.md`.
- Preserve source idea 252 unless durable source intent truly changes.
- Treat testcase-shaped matching, expectation weakening, helper/status/output
  renames, and classification-only maps claimed as implementation progress as
  route drift.
- If a later lifecycle step derives a code-changing implementation idea, that
  later idea must require build proof plus focused backend tests for the
  selected producer/source relation and compatibility rows.

## Step 1: Inventory Source-Producer Surfaces and Consumers

Goal: identify every prepared, AArch64, x86, riscv, and route/BIR surface
touching edge-publication source-producer lookup authority.

Primary target: prepared `edge_publication_source_producers` helpers,
source-producer records, helper/oracle names, and target consumers.

Actions:

- Inventory public prepared source-producer lookup helpers and observable
  status/helper rows.
- Identify prepared helper and AArch64 consumers that keep the helper surface
  compatibility-visible.
- Locate any route/BIR producer/source records or source identity facts that
  could serve as future authority.
- Classify x86 direct consumers, indirect consumers, or absence of consumers.
- Classify riscv direct consumers, indirect consumers, or absence of
  consumers.
- Record compatibility strings, helper/oracle names, fallback names,
  publication/output rows, and target-policy-sensitive rows that must remain
  stable.

Completion check:

- `todo.md` lists each relevant surface, consumer bucket, observed gap, and
  stable compatibility row without proposing implementation.

## Step 2: Select One Candidate Producer/Source Relation

Goal: choose the narrow producer/source relation to test for parity,
non-applicability, or an explicit blocker.

Primary target: the smallest source-producer relation that can map to prepared
`edge_publication_source_producers` lookup agreement.

Actions:

- Pick one producer/source relation with the best available cross-target
  evidence.
- Define what agreement means between route/BIR producer/source evidence and
  the prepared source-producer lookup answer.
- Separate semantic producer/source identity from publication availability,
  move selection, storage policy, instruction spelling, helper/oracle names,
  and output formatting.
- Name rows that remain target policy or compatibility, not semantic identity.

Completion check:

- `todo.md` states the selected relation, agreement rule, excluded policy
  surfaces, and why broader producer-index or publication parity is outside
  this plan.

## Step 3: Prove or Block x86 Evidence

Goal: determine whether x86 has direct or indirect evidence for the selected
producer/source relation, explicit non-applicability, or a named blocker.

Primary target: x86 prepared publication lookup/status, dispatch/module
output, route-debug, and backend emission consumers related to source
producers.

Actions:

- Trace x86 use of prepared source-producer lookup/status data for the
  selected relation.
- Check whether x86 consumes corresponding route/BIR producer/source evidence
  directly, indirectly, or not at all.
- Classify exact x86 dispatch/status/module-output rows that must remain
  compatibility-owned.
- Record the blocker or explicit non-applicability evidence if x86 parity is
  missing.

Completion check:

- `todo.md` records x86 as proven, blocked, or non-applicable for the selected
  relation with concrete file/function/test evidence.

## Step 4: Recheck Riscv Evidence Against the Same Relation

Goal: align riscv evidence with the selected producer/source relation instead
of nearby Route 5 or target-emission behavior.

Primary target: riscv prepared publication/source-producer statuses, fallback
rows, output rows, and route/BIR producer/source diagnostics.

Actions:

- Trace riscv consumption or publication of the selected producer/source
  relation.
- Confirm which riscv rows prove identity, block identity, or only prove
  target policy or compatibility.
- Preserve exact instruction text, fallback/status names, helper/oracle names,
  carrier/helper selection, and emission-policy output as compatibility or
  target policy.
- Record any riscv mismatch, unsupported, prepared-only, fallback, or
  policy-sensitive blockers.

Completion check:

- `todo.md` records riscv evidence for the same selected relation and flags
  all non-semantic output rows.

## Step 5: Build the Producer/Source Fail-Closed Matrix

Goal: define the cases required before any source-producer adapter can claim
semantic agreement.

Primary target: duplicate, conflict, mismatch, absent, unsupported,
prepared-only, fallback, and policy-sensitive producer/source rows.

Actions:

- For each fail-closed case, name the prepared compatibility surface that must
  stay observable.
- State the required route/BIR producer/source agreement or rejection
  behavior.
- Mark which cases are common and which are target-specific.
- Identify tests or existing evidence that would prove the matrix without
  weakening expectations.

Completion check:

- `todo.md` contains a matrix sufficient for reviewer rejection of expectation
  weakening, named-case shortcuts, helper/status rewrites, or old-failure
  retention.

## Step 6: Decide Producer-Adapter Readiness and Close or Split

Goal: conclude whether idea 252 proves a safe later implementation idea or
remains blocked.

Primary target: source-idea acceptance criteria and reviewer reject signals.

Actions:

- Summarize whether one source-producer relation is proven, blocked, or
  explicitly non-applicable for x86 and riscv.
- If blocked, name the exact missing consumer, evidence, or fail-closed row.
- If safe, propose a separate narrow implementation idea rather than expanding
  this blocker map.
- Preserve prepared source-producer lookup authority, helper/oracle names,
  compatibility strings, fallback names, and target output boundaries in the
  closure notes.

Completion check:

- Plan owner can close idea 252 as a completed blocker map, optionally with a
  new bounded follow-up idea, after close-time regression/lifecycle checks.
