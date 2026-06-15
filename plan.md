# 272 Phase F5 RISC-V Memory Accesses Prepared-Only Fail-Closed Proof

Status: Active
Source Idea: ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md

## Purpose

Prove one same-consumer prepared-only fail-closed row for
`PreparedFunctionLookups::memory_accesses` using the supported RISC-V dynamic
stack-source `LoadLocal` fixture from idea 271.

## Goal

Show that a prepared memory row can remain internally coherent while the
backend consumer fails closed when matching Route 3/Route 5 semantic authority
is absent or non-agreeing.

## Core Rule

Do not demote, delete, privatize, wrap, hide, rename, or weaken
`PreparedFunctionLookups::memory_accesses`; this runbook is a bounded proof
packet, not a public API migration.

## Read First

- `ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md`
- Existing RISC-V prepared edge publication tests for the idea 271 dynamic
  stack-source `LoadLocal` fixture.
- Existing prepared lookup helper/oracle tests that expose
  `PreparedFunctionLookups::memory_accesses` behavior.

## Current Targets

- RISC-V backend same-consumer path that directly reads
  `PreparedFunctionLookups::memory_accesses`.
- Dynamic stack-source `LoadLocal` fixture from idea 271.
- Compatibility output that must remain stable: `lw a1, 12(s2)`.
- Helper/oracle, prepared-printer, route/debug, fallback, status, wrapper, and
  baseline-visible outputs adjacent to the selected row.

## Non-Goals

- Do not add x86 memory-access proof.
- Do not claim whole-API demotion readiness for `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not broaden into stale-publication, byte-offset drift, or
  cross-publication mismatch proof unless the selected row naturally requires
  it.
- Do not move target-owned stack, storage, addressing, source-home, register,
  layout, formatting, emission, instruction spelling, or exact output policy
  into BIR.
- Do not weaken expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baselines.

## Working Model

- The positive path from idea 271 provides a supported prepared memory row for
  the RISC-V dynamic stack-source `LoadLocal` path.
- This proof must exercise the same real backend consumer, not only helper or
  oracle code.
- The failing row should invalidate or remove Route 3/Route 5 semantic
  agreement while keeping prepared lookup construction normal and internally
  coherent.
- If that row cannot be expressed without hand-built stale prepared state, stop
  this runbook and close or replace it with a narrower testability idea.

## Execution Rules

- Keep routine progress in `todo.md`.
- For each code-changing step, preserve the positive idea 271 path before
  adding or asserting fail-closed behavior.
- Treat expectation downgrades, unsupported-status rewrites, named-case
  shortcuts, or helper-only proof as route drift.
- Record exact proof commands and results in `todo.md`.
- Use the suggested proof scope unless the local build layout requires a
  different build directory or target name:

```bash
cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test
ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure
```

## Ordered Steps

### Step 1: Identify Same-Consumer Prepared Row Facts

Goal: establish the exact proof row before editing code or tests.

Primary targets:

- RISC-V backend consumer of `PreparedFunctionLookups::memory_accesses`.
- Idea 271 dynamic stack-source `LoadLocal` fixture and prepared memory row.
- Route 3 memory/source authority fact for that row.
- Route 5 publication/source owner, if relevant.

Actions:

- Inspect the RISC-V prepared edge publication fixture and its backend
  consumer path.
- Name the concrete prepared memory row built by normal prepared lookup
  construction.
- Name the Route 3 and Route 5 authority facts that currently agree with the
  positive path.
- Identify the prepared-only mutation or fixture condition that removes or
  invalidates that semantic agreement while leaving the prepared memory row
  internally coherent.
- Record the exact public compatibility output, status strings, helper/oracle
  rows, fallback behavior, route/debug output, prepared-printer output, and
  baseline-visible output that must remain byte-stable.

Completion check:

- `todo.md` names the consumer, prepared row, Route 3/Route 5 facts, proposed
  prepared-only fail-closed condition, and stable output contracts.
- If no normal prepared-only row is possible, `todo.md` records the blocker and
  recommends a narrower fixture or testability idea instead of forcing
  synthetic stale state.

### Step 2: Add or Expose the Prepared-Only Fail-Closed Row

Goal: express the selected prepared-only condition through normal fixture or
lookup construction.

Primary targets:

- RISC-V prepared edge publication test coverage.
- Prepared lookup helper/oracle surface only as supporting evidence.

Actions:

- Add the smallest test fixture or mutation needed to produce the prepared-only
  row identified in Step 1.
- Keep the positive agreement path from idea 271 intact.
- Assert fail-closed behavior at the real RISC-V backend consumer.
- Use helper/oracle assertions only to explain or guard the prepared lookup
  state, not as a substitute for backend same-consumer proof.

Completion check:

- The backend same-consumer test fails closed for the prepared-only row.
- The positive RISC-V output still includes byte-stable `lw a1, 12(s2)`.
- No expectation, status, fallback, route/debug, prepared-printer, wrapper, or
  baseline output is weakened.

### Step 3: Preserve Adjacent Compatibility Surfaces

Goal: prove that the fail-closed row does not destabilize public compatibility
or helper/oracle behavior.

Primary targets:

- `backend_riscv_prepared_edge_publication_test`
- `backend_prepared_lookup_helper_test`
- Any directly adjacent prepared-printer, route/debug, fallback, or baseline
  assertions touched by Step 2.

Actions:

- Run the targeted build for the touched test binaries.
- Run the narrow CTest scope for RISC-V prepared edge publication and prepared
  lookup helper coverage.
- If a nearby assertion changes, verify the change is a required semantic
  correction and not an output weakening.

Completion check:

- Exact build and CTest commands are recorded in `todo.md`.
- Narrow proof is green, or the blocker is recorded with exact failing tests
  and a route recommendation.

### Step 4: Close the Proof Packet Notes

Goal: leave the lifecycle state ready for supervisor review and eventual
source-idea closure.

Actions:

- Summarize which prepared-only PO-family blocker from idea 265 is reduced.
- Summarize which `memory_accesses` blockers remain.
- Record whether broader validation is recommended before closure.

Completion check:

- `todo.md` contains the proof summary, remaining blocker list, and exact
  validation evidence.
- The runbook can be reviewed without claiming broader demotion readiness than
  this one RISC-V same-consumer row supports.
