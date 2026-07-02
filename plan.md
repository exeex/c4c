# BIR Local-Memory Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/557_bir_local_memory_semantic_producer_admission.md

## Purpose

Repair BIR local-memory semantic producer admission for the largest current
exact `semantic lir_to_bir` lane without letting RV64/MIR infer missing
producer facts.

## Goal

Publish the local-memory semantic facts needed by load, GEP, store,
scalar/local-memory, and alloca admission families, then prove the current
representative RV64 rows advance for producer-owned reasons.

## Core Rule

Do not claim progress from named-case shortcuts, downstream fact inference,
diagnostic rewrites, expectation changes, unsupported downgrades, allowlist
changes, or weakened semantic admission. Progress must come from BIR producer
facts that generalize across the current local-memory families.

## Read First

- `ideas/open/557_bir_local_memory_semantic_producer_admission.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_outcome.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/memory/`
- `tests/backend/bir/`

## Current Targets

- Exact local-memory semantic rows: `264`.
- Topic counts: `79` load, `62` GEP, `58` store, `49`
  scalar/local-memory, and `16` alloca.
- Representative RV64 proof seeds:
  - `src/20000314-1.c` for load.
  - `src/20000717-4.c` for GEP.
  - `src/20001026-1.c` for store.
  - `src/20000519-1.c` for scalar/local-memory.
  - `src/20050604-1.c` for alloca.
- Primary implementation surfaces:
  `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/`.

## Non-Goals

- Do not implement call metadata, runtime/intrinsic, scalar/signature/control,
  or bootstrap/global data-shape lanes in this runbook.
- Do not repair RV64/MIR by guessing address, provenance, or memory facts
  missing from BIR.
- Do not weaken semantic admission, expectations, unsupported markers,
  allowlists, or runtime comparison behavior.
- Do not use one testcase shape as proof for all local-memory families.
- Do not fold unrelated aggregate, publication, or prepared-contract claims
  into this lane without fresh evidence.

## Working Model

- The source evidence already reconstructed and classified current rows.
- The first implementation packet should identify the concrete missing
  local-memory fact boundary and add focused BIR coverage before broadening.
- Representative RV64 rows are proof seeds, not the whole acceptance surface.
- If producer inspection proves one topic needs a separate boundary, stop and
  request plan-owner lifecycle split instead of silently expanding scope.

## Execution Rules

- Keep routine packet progress, commands, row seeds, and proof notes in
  `todo.md`.
- Each code-changing packet needs fresh build proof plus the exact focused
  command delegated by the supervisor.
- Add or update focused BIR tests before using RV64 representatives as proof.
- Use representative RV64 rows to confirm the current failure family moved for
  the producer reason.
- Escalate validation when shared memory helpers affect more than one
  local-memory family.
- If a proposed fix changes expectations, unsupported markers, allowlists, or
  runtime comparison behavior, reject the route and request supervisor review.

## Steps

### Step 1: Locate The Local-Memory Producer Boundary

Goal: identify the concrete BIR local-memory facts missing or malformed for
the current load, GEP, store, scalar/local-memory, and alloca families.

Actions:

- Inspect the representative row logs and the classified row table for the
  five local-memory topics.
- Inspect `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/` for the fact publication path used by
  each topic.
- Identify the first missing semantic fact or malformed BIR input per topic.
- Decide whether the five topics share one repair boundary or need a lifecycle
  split before implementation.

Completion check:

- `todo.md` names the concrete producer boundary, affected topics, and the
  focused tests or split request needed next.

### Step 2: Add Focused BIR Coverage

Goal: pin the local-memory producer contract in focused BIR tests before
repairing or broadening RV64 proof.

Actions:

- Add or extend focused BIR tests for the selected local-memory topics.
- Cover load, GEP, store, scalar/local-memory, and alloca unless Step 1
  justifies a narrower first packet.
- Keep assertions tied to producer facts rather than downstream target output.
- Run the delegated focused build and test command.

Completion check:

- Focused BIR tests expose the intended producer contract and are recorded in
  `todo.md` with command output.

### Step 3: Repair Semantic Fact Publication

Goal: make BIR publish the semantic local-memory facts required by the focused
tests and representative current rows.

Actions:

- Implement the minimal producer-side repair for the selected local-memory
  fact boundary.
- Preserve existing semantic admission checks; do not bypass them.
- Run focused BIR tests and the delegated build proof.
- Record any neighboring local-memory families intentionally left for later
  packets.

Completion check:

- The selected local-memory facts are published by BIR, focused tests pass,
  and `todo.md` records remaining local-memory families.

### Step 4: Prove Current RV64 Representative Rows

Goal: prove representative current RV64 rows advance for producer-owned
reasons after focused BIR coverage is green.

Actions:

- Run a narrow RV64 gcc_torture subset that includes representatives for every
  repaired local-memory topic.
- Inspect failures that remain in the same family and decide whether they are
  in-scope next packets or a separate boundary.
- Confirm no expectation, unsupported-marker, allowlist, or runtime comparison
  behavior changed.
- Ask the supervisor to escalate validation when multiple local-memory packets
  have landed or shared helpers changed broadly.

Completion check:

- `todo.md` records focused and RV64 proof, remaining rows or topics, and
  whether this source idea should continue, split, or close.
