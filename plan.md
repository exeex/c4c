# Phase F3 Route 3 Memory Source Parity Blocker Map

Status: Active
Source Idea: ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md

## Purpose

Turn idea 250 into an execution runbook for proving, or explicitly blocking,
one narrow Route 3 memory/source identity boundary before prepared
`memory_accesses` helpers can become compatibility mirrors.

## Goal

Decide whether one shared route/BIR memory/source identity has enough x86 and
riscv evidence to support a later bounded adapter, while preserving prepared
lookup/status compatibility and target-owned addressing/storage policy.

## Core Rule

Do not treat prepared memory lookup results as migrated semantic authority
unless the selected route/BIR memory/source fact is proven for the relevant
target evidence and the prepared answer only mirrors that fact or fails closed.

## Read First

- `ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md`
- Prepared lookup and status surfaces for `PreparedFunctionLookups::memory_accesses`
- Route 3 / BIR memory-source publication and consumer paths
- Existing x86 and riscv backend prepared-memory tests and route-debug/status tests

## Current Targets / Scope

- One selected memory/source identity fact from route/BIR publication to
  prepared memory lookup agreement.
- x86 evidence: either a concrete consumer for the selected fact or the exact
  consumer gap that blocks parity.
- riscv evidence: Route 3/status rows checked against the same semantic fact.
- Fail-closed cases for missing, invalid, duplicate/conflict, mismatch,
  unsupported, prepared-only, fallback, and policy-sensitive rows.

## Non-Goals

- Do not delete, privatize, rename, or bypass
  `PreparedFunctionLookups::memory_accesses`.
- Do not move addressing modes, frame/storage placement, load/store
  instruction choice, register materialization, or target output formatting
  into BIR.
- Do not rewrite memory baselines, prepared status strings, helper/oracle
  names, fallback names, or output rows.
- Do not perform broad memory lowering cleanup outside the selected
  memory/source identity.

## Working Model

- Route/BIR owns only the selected semantic memory/source identity if target
  evidence proves it.
- Prepared memory lookup/status surfaces remain public compatibility authority
  until an adapter can prove agreement and fail closed on disagreement.
- Target addressing and storage decisions remain target-owned even when the
  memory/source identity is route/BIR-backed.
- A blocked row is a valid result when it names the missing consumer, evidence,
  or fail-closed proof precisely.

## Execution Rules

- Prefer analysis packets first; do not implement an adapter in this plan
  unless a later lifecycle step explicitly creates an implementation idea.
- Keep routine inventory, matrix, and blocker notes in `todo.md`.
- Preserve source idea 250 unless durable intent truly changes.
- Treat testcase-shaped matching, expectation weakening, or status-string
  rewrites as route drift.
- For any code-changing packet that a future plan may derive, require build
  proof plus focused x86/riscv backend tests covering the selected fact and
  compatibility rows.

## Step 1: Inventory Memory-Source Surfaces and Consumers

Goal: identify the exact prepared, route/BIR, x86, and riscv surfaces involved
in Route 3 memory/source identity.

Primary target: lookup helpers, status rows, route/BIR memory-source records,
and backend consumers around prepared memory access handling.

Actions:

- Inventory `PreparedFunctionLookups::memory_accesses` helpers and public
  statuses.
- Locate Route 3 / BIR memory-source publication records and any helper
  accessors.
- Identify x86 consumers or absence of consumers for memory/source identity.
- Identify riscv consumers and status rows tied to the same fact family.
- Record helper/oracle names, fallback names, unsupported rows, and
  addressing/storage-sensitive output that must stay compatibility-owned.

Completion check:

- `todo.md` lists each relevant surface, consumer bucket, and observed gap
  without proposing implementation.

## Step 2: Select One Candidate Memory-Source Identity Fact

Goal: choose the narrow fact to test for parity or block explicitly.

Primary target: the smallest route/BIR memory/source relation that can map to
prepared memory lookup agreement.

Actions:

- Pick one memory/source identity fact that has the best available x86 and
  riscv evidence.
- Define what agreement means between route/BIR publication and prepared
  memory lookup answers.
- Separate semantic identity from addressing mode, storage, frame layout,
  register materialization, and output formatting.
- Name rows that remain target policy or compatibility, not semantic identity.

Completion check:

- `todo.md` states the selected fact, agreement rule, excluded policy surfaces,
  and why broader memory parity is outside this plan.

## Step 3: Prove or Block x86 Evidence

Goal: determine whether x86 has a concrete consumer for the selected
memory/source identity or a named blocker.

Primary target: x86 prepared memory lookup, route-debug, backend emission, and
status/debug consumers relevant to the selected fact.

Actions:

- Trace x86 use of prepared memory lookup/status data for the selected fact.
- Check whether x86 consumes the corresponding route/BIR fact directly,
  indirectly, or not at all.
- Classify exact x86 output/status rows that must remain compatibility-owned.
- Record the blocker if x86 evidence is missing or indirect.

Completion check:

- `todo.md` records x86 as proven, blocked, or non-applicable for the selected
  fact with concrete file/function/test evidence.

## Step 4: Recheck Riscv Evidence Against the Same Fact

Goal: align riscv Route 3/status evidence with the selected memory/source
identity instead of a nearby target-specific behavior.

Primary target: riscv prepared memory status, publication, and output rows
that Phase F2 treated as Route 3/source-memory evidence.

Actions:

- Trace riscv consumption or publication of the selected route/BIR fact.
- Confirm which riscv rows prove identity and which only prove target policy
  or compatibility.
- Preserve exact instruction text, fallback/status names, and
  addressing/storage-sensitive output as compatibility or target policy.
- Record any riscv mismatch, unsupported, prepared-only, or fallback blockers.

Completion check:

- `todo.md` records riscv evidence for the same selected fact and flags all
  non-semantic output rows.

## Step 5: Build the Fail-Closed Proof Matrix

Goal: define the cases required before any prepared-memory adapter can claim
semantic agreement.

Primary target: missing, invalid, duplicate/conflict, mismatch, unsupported,
prepared-only, fallback, and policy-sensitive memory/source rows.

Actions:

- For each fail-closed case, name the prepared compatibility surface that must
  stay observable.
- State the required route/BIR agreement or rejection behavior.
- Mark which cases are common and which are target-specific.
- Identify tests or existing evidence that would prove the matrix without
  weakening expectations.

Completion check:

- `todo.md` contains a matrix sufficient for reviewer rejection of expectation
  weakening, named-case shortcuts, or old-failure retention.

## Step 6: Decide Adapter Readiness and Close or Split

Goal: conclude whether idea 250 proves a safe later one-adapter implementation
idea or remains blocked.

Primary target: source-idea acceptance criteria and reviewer reject signals.

Actions:

- Summarize whether one shared route/BIR memory/source identity is proven for
  named x86 and riscv evidence.
- If blocked, name the exact missing consumer, evidence, or fail-closed row.
- If safe, propose a separate narrow implementation idea rather than expanding
  this blocker map.
- Preserve prepared lookup/status authority and target policy boundaries in
  the closure notes.

Completion check:

- Plan owner can close idea 250 as a completed blocker map, optionally with a
  new bounded follow-up idea, after close-time regression/lifecycle checks.
