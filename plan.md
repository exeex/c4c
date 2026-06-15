# Phase F5 Memory Accesses Byte-Offset Coverage Gate Runbook

Status: Active
Source Idea: ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md

## Purpose

Decide and prove the next bounded byte-offset coverage step for
`PreparedFunctionLookups::memory_accesses` after the selected RISC-V
same-consumer row from idea 273.

## Goal

Inventory remaining byte-offset drift surfaces, pick one bounded next proof row
or fixture blocker, and preserve compatible exact output if a backend proof is
supportable.

## Core Rule

Do not claim exhaustive byte-offset coverage from one narrow row. Any accepted
proof must exercise a real target consumer and keep fallback, status/debug,
route-debug, helper/oracle, prepared-printer, wrapper, exact-output, and
baseline contracts at least as strong as before.

## Read First

- `ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- Prior `memory_accesses` Route 3 / Route 5 helper and oracle coverage near the
  prepared edge publication tests

## Current Targets / Scope

- `PreparedFunctionLookups::memory_accesses`
- Remaining byte-offset drift surfaces after the selected RISC-V same-consumer
  proof from idea 273
- One bounded next proof row, or an explicit fixture-support blocker if no
  real consumer proof is currently supportable

## Non-Goals

- Do not fold in stale-publication, cross-publication, x86 parity,
  edge-publication families, metadata, liveness, aggregate retirement, or draft
  155 work.
- Do not rewrite broad target memory lowering.
- Do not downgrade supported-path tests to unsupported or weaken exact-output
  contracts.
- Do not use helper-only assertions as a substitute for backend consumer proof.

## Working Model

- The gate is a coverage decision point, not an exhaustive completion claim.
- The first packet should name remaining byte-offset drift surfaces and choose
  either one supportable proof row or a fixture-support blocker.
- If a proof row is chosen, implementation should stay narrow and preserve the
  compatible positive path output.
- Closure notes must state which byte-offset surfaces remain unsupported,
  unexamined, or proven.

## Execution Rules

- Keep routine progress in `todo.md`.
- Prefer narrow test proof for the selected backend consumer before broader
  validation.
- Treat testcase-shaped matching, named-case-only branches, expectation
  rewrites, and output relaxation as route drift.
- If the inventory shows a separate fixture-support prerequisite, stop and
  report it instead of silently expanding this runbook.

## Steps

### Step 1: Inventory Remaining Byte-Offset Surfaces

Goal: identify the remaining byte-offset drift surfaces that matter to
`memory_accesses` proof completeness.

Concrete actions:

- Inspect existing RISC-V `memory_accesses` prepared edge publication tests and
  related Route 3 / Route 5 helper coverage.
- Separate already-proven same-consumer RISC-V rows from unexamined public
  lookup consumers, target operand surfaces, wrapper/exact-output contracts,
  and any synthetic prepared offset drift rows.
- Record the selected next proof candidate or explicit fixture-support blocker
  in `todo.md`.

Completion check:

- `todo.md` names the relevant remaining surfaces and identifies exactly one
  next bounded proof row or a fixture-support blocker.

### Step 2: Prove The Selected Byte-Offset Drift Row

Goal: if Step 1 found a supportable row, exercise it through a real backend
consumer and prove fail-closed behavior.

Concrete actions:

- Add or extend a focused backend test for the selected byte-offset drift row.
- Drive the proof through the real target consumer path, not only helper-level
  predicates.
- Assert the unsupported/fail-closed result, no relaxed fallback output, and
  unchanged route agreement expectations that remain relevant to the row.

Completion check:

- The selected mismatch fails closed through the real consumer path, and no
  contract is weakened to make the test pass.

### Step 3: Preserve Compatible Exact Output

Goal: prove that the compatible positive path for the same consumer still emits
the expected target instruction.

Concrete actions:

- Keep or add a positive-path assertion adjacent to the fail-closed row.
- Assert exact output for the compatible byte-offset case.
- Avoid relaxing wrapper, prepared-printer, route-debug, helper/oracle, or
  baseline expectations.

Completion check:

- The compatible case still emits the exact expected instruction, and the
  negative row does not change that contract.

### Step 4: Run Acceptance Proof

Goal: produce fresh proof for the bounded gate.

Concrete actions:

- Run the supervisor-delegated build and targeted CTest command for the edited
  backend test bucket.
- Write proof output to `test_after.log` if delegated by the supervisor.
- Record the exact command and result in `todo.md`.

Completion check:

- Build and targeted test proof pass, or `todo.md` records the exact blocker.

### Step 5: Record Coverage Remainder

Goal: leave clear closure notes for what this byte-offset gate did and did not
cover.

Concrete actions:

- Update `todo.md` with the proven row, unsupported or unexamined byte-offset
  surfaces, and any separate fixture-support follow-up.
- Do not rewrite the source idea unless the durable source intent changes or a
  separate initiative must be created under `ideas/open/`.

Completion check:

- A future plan owner can decide close, deactivate, or split without
  rediscovering which byte-offset surfaces remain.
