# BIR Local-Memory And Call-Metadata Boundary Review Runbook

Status: Active
Source Idea: ideas/open/547_bir_local_memory_call_metadata_boundary_review.md

## Purpose

Turn the current local-memory and call-metadata evidence gap into owned,
reviewable follow-up work without letting RV64 lowering guess missing producer
facts.

## Goal

Classify current `unsupported_local_memory_access` rows and reconstruct any
current call-metadata rows before implementation ideas are created.

## Core Rule

Do not implement RV64 memory or call lowering that infers missing BIR/prepared
facts from target-specific shapes. First ownership must be proven from current
logs and focused reproduction.

## Read First

- `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`
- Current RV64 gcc torture backend logs under `build/agent_state/` and
  `build/rv64_gcc_c_torture_backend/`, if present
- Existing BIR/prepared tests for local memory, address provenance, call
  arguments, return facts, `memcpy`, and `memset`

## Current Targets

- Current `unsupported_local_memory_access` rows.
- Any current call-metadata failure rows with row-level evidence.
- Producer facts for address provenance, GEP, load/store, call argument,
  return, `memcpy`, and `memset` boundaries.

## Non-Goals

- Do not repair RV64 lowering in this runbook.
- Do not weaken tests, expectations, unsupported markers, or accounting.
- Do not treat call-metadata suspicions as implementation-ready without current
  row evidence.
- Do not rework unrelated RV64 instruction families.
- Do not merge independent producer gaps into one implementation route.

## Working Model

- `unsupported_local_memory_access` may include BIR producer gaps, prepared
  contract gaps, RV64 consumer gaps, and stale evidence.
- Call metadata must be reconstructed from current rows before it becomes a
  queue.
- This runbook should end by creating or requesting narrow follow-up ideas only
  after first ownership is proven.

## Execution Rules

- Keep routine progress in `todo.md`; do not edit the source idea unless the
  durable source intent changes or a separate initiative must be recorded.
- Prefer focused reproduction over broad scans when classifying a family.
- Preserve exact commands, representative rows, and first bad facts in
  `todo.md`.
- Treat diagnostic renames, expectation rewrites, and target-shaped recovery as
  non-progress.
- For any code-changing follow-up proposed from this review, require focused
  BIR/prepared coverage first, then representative RV64 proof.

## Steps

### Step 1: Reconstruct Local-Memory Evidence

Goal: establish the current row set and first-owner candidates for
`unsupported_local_memory_access`.

Primary target: current RV64 gcc torture backend artifacts and representative
case logs.

Actions:
- Inspect current summary files and failed-row logs for
  `unsupported_local_memory_access`.
- Record the current row count, representative case names, and first bad fact
  shape in `todo.md`.
- Group rows by missing fact category: address provenance, GEP, load/store,
  frame/global/local object fact, `memcpy`, `memset`, prepared publication, RV64
  consumer evidence gap, or stale row.
- Identify the smallest representative reproduction command for each retained
  group.

Completion check:
- `todo.md` lists the current row evidence, group names, representative rows,
  and which owner still needs proof.

### Step 2: Reconstruct Call-Metadata Evidence

Goal: determine whether current call-metadata rows exist and what facts they
lack.

Primary target: current RV64 gcc torture backend artifacts and any prepared or
BIR call lowering diagnostics.

Actions:
- Search current logs for call argument, call return, variadic, helper-call,
  aggregate-call, and call-site metadata diagnostics.
- Separate verified rows from no-count suspicions.
- For verified rows, record representative cases, diagnostics, and suspected
  producer boundary in `todo.md`.
- If no current call-metadata rows are found, record the evidence and keep call
  metadata out of implementation scope.

Completion check:
- `todo.md` either lists current call-metadata row groups with representative
  evidence or explicitly records that no implementation-ready call-metadata row
  set exists.

### Step 3: Inspect Producer Surfaces For Retained Groups

Goal: prove whether each retained group is BIR-owned, prepared-owned,
RV64-owned, or still an evidence gap.

Primary target: BIR/prepared producer code and focused tests adjacent to the
retained row groups.

Actions:
- Inspect existing producer surfaces for the retained local-memory and
  call-metadata groups.
- Prefer AST-backed symbol queries for large C++ files when helpful.
- Add no implementation changes in this review step; document missing or
  already-present facts in `todo.md`.
- Mark each group with first owner and the minimal test surface needed by a
  future implementation idea.

Completion check:
- Every retained group has a first-owner classification or a concrete evidence
  gap, plus the focused proof surface a follow-up would need.

### Step 4: Split Or Close The Review

Goal: convert proven owner groups into durable follow-up ideas or conclude that
the current evidence does not justify implementation.

Primary target: lifecycle artifacts only, unless the supervisor delegates a
separate source-idea creation packet.

Actions:
- For each coherent implementation-ready group, request or create a separate
  source idea with concrete reviewer reject signals.
- Keep unrelated groups separate.
- If no group is implementation-ready, record why the review closes without a
  code route.
- Validate lifecycle state with `git diff --check` and plan-review state
  alignment.

Completion check:
- The review has either produced narrow follow-up ideas or recorded a
  WAIT_FOR_NEW_IDEA-compatible conclusion for this source idea.
