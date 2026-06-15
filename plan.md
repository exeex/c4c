# Phase F5 x86 memory_accesses Public-Field Parity Gate Runbook

Status: Active
Source Idea: ideas/open/279_phase_f5_x86_memory_accesses_public_field_parity_gate.md

## Purpose

Determine whether x86 has a real public
`PreparedFunctionLookups::memory_accesses` consumer boundary comparable to the
recent RISC-V public-field proofs.

## Goal

Name the x86 consumer surface examined, then either prove one bounded x86
public-field row through a real consumer or record explicit fail-closed
non-applicability with the remaining blocker.

## Core Rule

Do not claim x86 public-field parity from RISC-V-only evidence, helper-only
formatting, or a renamed wrapper. Any accepted proof must exercise a real x86
consumer path and keep fallback, status/debug, route-debug, helper/oracle,
prepared-printer, wrapper, exact-output, and baseline contracts at least as
strong as before.

## Read First

- `ideas/open/279_phase_f5_x86_memory_accesses_public_field_parity_gate.md`
- `src/backend/mir/x86/module/module.cpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/prealloc/addressing.hpp`

## Current Targets / Scope

- x86 use of `PreparedFunctionLookups::memory_accesses`
- Adjacent x86 prepared edge-publication and Route 3 / Route 5 agreement paths
- One bounded x86 public-field proof row, or a precise non-applicability /
  fixture-support blocker if no real x86 public-field consumer exists

## Non-Goals

- Do not claim x86 parity from RISC-V-only proof rows.
- Do not migrate x86 consumers, privatize prepared APIs, retire aggregate
  prepared data, or hide prepared APIs behind adapters.
- Do not fold in RISC-V stale/cross/byte-offset rows,
  `edge_publications`, `edge_publication_source_producers`, metadata,
  liveness, aggregate retirement, or draft 155 work.
- Do not downgrade supported-path tests to unsupported or weaken exact-output
  contracts.
- Do not use helper-only assertions as a substitute for backend consumer proof.

## Working Model

- Existing RISC-V proofs establish real public `memory_accesses` lookup
  behavior for RISC-V, not x86 parity.
- Existing x86 prepared paths appear centered on edge-publication source homes,
  copied publications, and addressing records; this gate must verify whether
  any x86 path directly consumes the public `memory_accesses` field.
- If no real public-field consumer exists, the correct outcome is explicit
  fail-closed non-applicability, not silent parity.

## Execution Rules

- Keep routine progress and packet notes in `todo.md`.
- Prefer an inspection packet first; do not add tests until a real x86 consumer
  boundary or precise blocker is named.
- If a supportable x86 row exists, drive it through the real x86 consumer path
  and preserve exact compatible output.
- Treat testcase-shaped matching, named-case-only branches, expectation
  rewrites, output relaxation, and unsupported downgrades as route drift.
- If the inspection finds a separate fixture-support prerequisite, stop and
  record it instead of silently expanding this runbook.

## Steps

### Step 1: Inspect x86 memory_accesses Consumers

Goal: determine whether x86 has a real public
`PreparedFunctionLookups::memory_accesses` consumer boundary.

Concrete actions:

- Inspect x86 prepared lowering and module paths for direct or indirect access
  to `lookups->memory_accesses`.
- Compare x86 edge-publication source-home behavior against the RISC-V
  `memory_accesses` public-field consumer fixture.
- Separate real backend consumers from helper-only lookup tests, prepared
  printers, wrappers, copied-publication paths, and addressing-only records.
- Record in `todo.md` the consumer surface examined and whether a real
  public-field boundary exists.

Completion check:

- `todo.md` names the examined x86 surface and chooses exactly one next bounded
  proof row or an explicit non-applicability / fixture-support blocker.

### Step 2: Prove Or Block The x86 Public-Field Row

Goal: either exercise the selected x86 public-field row through a real backend
consumer or record why no such row is currently supportable.

Concrete actions:

- If Step 1 found a real consumer, add or extend a focused x86 backend test for
  the selected public `memory_accesses` row.
- Drive the proof through the real x86 target path, not only helper-level
  predicates.
- Assert fail-closed behavior for the mismatched or unsupported public row, and
  keep relevant Route 3 / Route 5 agreement expectations intact.
- If no real consumer exists, update `todo.md` with the exact absent boundary
  and smallest follow-up fixture-support or implementation prerequisite.

Completion check:

- The selected row fails closed through a real x86 consumer, or `todo.md`
  records explicit non-applicability with a precise blocker.

### Step 3: Preserve Compatible x86 Output Or Fallback

Goal: prove that the compatible x86 path remains unchanged for the examined
consumer surface.

Concrete actions:

- Keep or add a positive-path assertion adjacent to any negative row.
- Assert exact x86 output when a supportable public-field consumer exists.
- If Step 2 records non-applicability, preserve the relevant existing x86
  fallback or copied-publication behavior without weakening contracts.
- Avoid relaxing wrapper, prepared-printer, route-debug, helper/oracle, or
  baseline expectations.

Completion check:

- Compatible exact output or fallback behavior remains stable, and the gate
  does not change unrelated x86 lowering behavior.

### Step 4: Run Acceptance Proof

Goal: produce fresh proof for the bounded x86 parity gate.

Concrete actions:

- Run the supervisor-delegated build and targeted CTest command for the
  examined x86 test bucket.
- Write proof output to `test_after.log` if delegated by the supervisor.
- Record the exact command and result in `todo.md`.

Completion check:

- Build and targeted test proof pass, or `todo.md` records the exact blocker.

### Step 5: Record Gate Outcome

Goal: leave clear lifecycle evidence for close, deactivate, or follow-up
planning.

Concrete actions:

- Update `todo.md` with the x86 consumer surface examined, the proven row or
  non-applicability decision, and any remaining x86 boundary blocker.
- Do not rewrite the source idea unless the durable source intent changes or a
  separate initiative must be created under `ideas/open/`.

Completion check:

- A future plan owner can decide close, deactivate, or split without
  rediscovering whether x86 public-field parity exists.
