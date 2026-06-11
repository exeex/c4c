# Route 3 Memory/Source Identity Adapter Runbook

Status: Active
Source Idea: ideas/open/202_route3_memory_source_identity_adapter.md

## Purpose

Activate the bounded Route 3 follow-up from the Phase B2 schema-readiness
audit: migrate one selected memory/source identity reader to BIR Route 3
semantic identity while retaining prepared target-addressing fallback.

## Goal

Land one non-regressive Route 3 memory/source identity adapter with explicit
success, rejection, ambiguity, and fallback proof.

## Core Rule

Route 3 may provide only semantic memory/source identity. Prepared
target-addressing lookup remains authoritative for address formation,
frame/global/TLS relocation, materialization, addressing-mode legality, final
operands, and policy-sensitive behavior.

## Read First

- `ideas/open/202_route3_memory_source_identity_adapter.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`

## Current Scope

- Select exactly one direct memory/source identity reader.
- Use Route 3 records or indexes for memory access id, result or stored-value
  identity, semantic base/source identity, and block/instruction compatibility.
- Preserve prepared fallback for target-addressing, address formation,
  materialization, and policy-sensitive cases.
- Add or update proof for absent Route 3 facts, mismatched route/prepared
  facts, non-memory cases, alias/address ambiguity, and fallback behavior.

## Non-Goals

- Do not replace all `memory_accesses` or `PreparedMemoryAccessLookups`.
- Do not move frame offsets, relocation spelling, concrete layout,
  addressing-mode legality, volatile/address-space lowering policy,
  materialization, or final operands into BIR schema.
- Do not retire prepared memory/frame/stack or target-addressing oracles.
- Do not refresh expected output or baselines as proof of ownership.

## Working Model

- BIR Route 3 records are identity inputs, not target-addressing policy.
- The first implementation slice should pick one reader and prove the
  route/prepared agreement boundary before any broader migration.
- Missing, conflicting, ambiguous, or non-memory Route 3 facts must fail closed
  or fall back exactly where the selected reader contract requires.

## Execution Rules

- Keep each code slice bounded to one selected reader or adapter boundary.
- Name retained prepared fallback explicitly in code comments or proof notes
  only where that prevents accidental promotion of policy into BIR.
- Use semantic route/index checks instead of fixture-name or expected-string
  shortcuts.
- Preserve existing target output unless the source idea is explicitly updated
  by the plan owner.
- For code-changing steps, run fresh build or compile proof plus the delegated
  targeted test subset before marking the step complete.

## Step 1: Select Reader And Baseline Proof

Goal: identify one direct memory/source identity reader and define the narrow
proof command for the first adapter slice.

Primary targets:

- memory/source identity consumers that currently read prepared memory facts
- tests covering the selected reader's success and failure modes

Actions:

- Inspect direct memory/source identity readers and choose one bounded reader.
- Record why the reader uses Route 3 semantic identity rather than
  target-addressing policy.
- Identify existing tests or fixtures for success, missing facts, mismatches,
  non-memory inputs, ambiguity, and prepared fallback.
- Run or delegate a baseline command for the selected proof subset before code
  changes when the supervisor asks for regression-log pairing.

Completion check:

- `todo.md` names the selected reader, retained prepared fallback, proof subset,
  and any missing test coverage required before implementation.

## Step 2: Add Route 3 Identity Adapter

Goal: introduce the smallest adapter needed for the selected reader to obtain
Route 3 semantic memory/source identity from BIR.

Primary targets:

- Route 3 record/index lookup helpers near existing BIR backend lookup code
- the selected reader's immediate identity lookup boundary

Actions:

- Add a narrow helper or adapter that validates block/instruction
  compatibility and memory access identity.
- Return semantic identity only: memory access id, result or stored-value
  identity, and semantic base/source identity.
- Reject absent, mismatched, non-memory, duplicate, conflict, or ambiguous
  Route 3 facts according to the selected reader contract.
- Keep prepared lookup calls in place for target-addressing and
  policy-sensitive facts.

Completion check:

- The adapter compiles, is used only by the selected reader path, and cannot
  supply address formation, materialization, or final operand policy.

## Step 3: Wire The Selected Reader

Goal: migrate the selected reader to consume Route 3 semantic identity while
preserving prepared fallback behavior.

Primary targets:

- the selected memory/source identity reader
- any local wrapper needed to compare Route 3 identity with prepared facts

Actions:

- Replace only the selected reader's semantic identity source with the Route 3
  adapter.
- Keep prepared target-addressing lookup authority for all out-of-scope facts.
- Fail closed or fall back on missing, mismatched, ambiguous, non-memory, or
  alias/address ambiguity cases.
- Avoid helper privatization, broad memory lookup replacement, or unrelated
  cleanup.

Completion check:

- The selected reader obtains only Route 3 semantic identity from BIR, and all
  target-addressing or policy-sensitive behavior still flows through prepared
  lookup authority.

## Step 4: Prove Success, Rejection, And Fallback

Goal: prove the adapter boundary is real and non-regressive.

Primary targets:

- focused unit or integration tests for the selected reader
- existing target-output or baseline checks affected by the selected path

Actions:

- Add or update proof for successful Route 3 semantic identity use.
- Prove absent Route 3 facts, mismatched route/prepared facts, non-memory
  cases, duplicate/conflict or ambiguity, alias/address ambiguity, and prepared
  fallback behavior.
- Preserve non-regressive target output for affected AArch64 behavior.
- Do not use expected-string rewrites, unsupported downgrades, or weakened
  guards as proof.

Completion check:

- The delegated proof subset is green and covers success, absence, mismatch,
  non-memory, ambiguity, prepared fallback, and output stability where relevant.

## Step 5: Broader Validation And Handoff

Goal: decide whether the first Route 3 adapter slice is ready for review,
follow-up expansion, or closure of this source idea.

Actions:

- Run the supervisor-delegated broader check if the adapter touches shared BIR
  lookup behavior or target output.
- Update `todo.md` with the exact retained prepared oracle or fallback that
  remains after the adapter lands.
- Record whether additional Route 3 readers are intentionally out of scope for
  this idea or need a new open idea.

Completion check:

- The implementation and proof satisfy
  `ideas/open/202_route3_memory_source_identity_adapter.md`, with no broad
  helper replacement, no target-addressing policy migration into BIR, and no
  testcase-overfit evidence.
