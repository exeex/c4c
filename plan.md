# 273 Phase F5 RISC-V Memory Accesses Byte-Offset Drift Fail-Closed Proof

Status: Active
Source Idea: ideas/open/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md

## Purpose

Prove one RISC-V same-consumer byte-offset drift fail-closed row for
`PreparedFunctionLookups::memory_accesses` using the supported dynamic
stack-source `LoadLocal` path.

## Goal

Show that the prepared memory row remains observable while Route 3 memory/source
authority disagrees only on byte offset, and that this mismatch is not accepted
as semantic agreement.

## Core Rule

Do not demote, delete, privatize, wrap, hide, rename, or weaken
`PreparedFunctionLookups::memory_accesses`; this runbook is a bounded proof
packet, not a public API migration or demotion-readiness claim.

## Read First

- `ideas/open/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md`
- Closed proof notes for ideas 265 and 272 when context is needed.
- Existing RISC-V prepared edge publication tests for the dynamic stack-source
  `LoadLocal` fixture.
- Existing prepared lookup helper/oracle tests that expose
  `PreparedFunctionLookups::memory_accesses` behavior.

## Current Targets

- RISC-V backend consumer that directly reads
  `PreparedFunctionLookups::memory_accesses`.
- Dynamic stack-source `LoadLocal` prepared memory row.
- Matching positive Route 3 memory/source authority fact.
- One drift row where the prepared memory byte offset and Route 3 byte offset
  disagree.
- Compatibility output that must remain stable: `lw a1, 12(s2)`.
- Helper/oracle, prepared-printer, route/debug, fallback, status, wrapper, and
  baseline-visible outputs adjacent to the selected row.

## Non-Goals

- Do not repeat the prepared-only proof covered by idea 272.
- Do not add x86 memory-access proof.
- Do not add stale-publication or cross-publication mismatch proof unless it is
  the minimal supported way to express the byte-offset drift row.
- Do not claim whole-API demotion readiness for `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not move target-owned stack, storage, addressing, source-home, register,
  layout, wrapper, formatting, emission, instruction spelling, or exact output
  policy into BIR.
- Do not weaken expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baselines.

## Working Model

- Idea 272 proved the RISC-V consumer directly reads
  `PreparedFunctionLookups::memory_accesses` and preserved the positive
  `LoadLocal` output.
- This packet must reuse that same real backend consumer, not only helper or
  oracle surfaces.
- The positive path should keep the prepared row and Route 3 fact in agreement.
- The drift row should disagree only on byte offset and must fail closed as a
  Route 3 semantic agreement candidate.
- If the drift row cannot be expressed through supported fixture construction
  without hand-built stale prepared state, stop this runbook and recommend a
  narrower fixture or testability idea.

## Execution Rules

- Keep routine progress, blockers, and proof results in `todo.md`.
- Complete Step 1 before editing code or tests.
- Preserve the positive RISC-V agreement path before adding or asserting
  fail-closed behavior.
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

### Step 1: Identify Byte-Offset Drift Row Facts

Goal: establish the exact same-consumer proof row before editing code or tests.

Primary targets:

- RISC-V backend consumer of `PreparedFunctionLookups::memory_accesses`.
- Dynamic stack-source `LoadLocal` fixture and prepared memory row.
- Matching positive Route 3 memory/source authority fact.
- Prepared byte offset, Route 3 positive byte offset, and Route 3 drift byte
  offset.
- Expected fail-closed status or agreement flag for the drift row.
- Public compatibility output that must remain byte-stable.

Actions:

- Inspect the RISC-V prepared edge publication fixture and backend consumer
  path.
- Name the concrete prepared memory row built by normal prepared lookup
  construction.
- Name the matching positive Route 3 memory/source authority fact and its byte
  offset.
- Identify the drift byte offset that changes only Route 3 authority while the
  prepared row remains present.
- Identify the expected fail-closed status or agreement flag that must reject
  the mismatch as semantic agreement.
- Record the public compatibility output, especially `lw a1, 12(s2)`, plus
  adjacent status strings, helper/oracle rows, fallback behavior, route/debug
  output, prepared-printer output, wrapper output, and baseline-visible output
  that must remain stable.

Completion check:

- `todo.md` names the consumer, prepared row, positive Route 3 fact, prepared
  byte offset, positive Route 3 byte offset, drift Route 3 byte offset,
  fail-closed status or agreement flag, and stable compatibility outputs.
- If no supported fixture route can express the drift row without hand-built
  stale prepared state, `todo.md` records the blocker and recommends a narrower
  fixture or testability idea instead.

### Step 2: Add or Expose the Byte-Offset Drift Row

Goal: express the selected byte-offset drift through supported fixture or
lookup construction.

Primary targets:

- RISC-V prepared edge publication test coverage.
- Prepared lookup helper/oracle surface only as supporting evidence.

Actions:

- Add the smallest fixture mutation or test row needed to produce the Step 1
  byte-offset drift.
- Keep the positive agreement path intact.
- Assert fail-closed behavior at the real RISC-V backend consumer.
- Use helper/oracle assertions only to explain or guard prepared lookup and
  Route 3 authority state, not as a substitute for backend same-consumer proof.

Completion check:

- The backend same-consumer test fails closed for the byte-offset drift row.
- The positive RISC-V output still includes byte-stable `lw a1, 12(s2)`.
- No expectation, status, fallback, route/debug, prepared-printer, wrapper, or
  baseline output is weakened.

### Step 3: Preserve Adjacent Compatibility Surfaces

Goal: prove that the fail-closed row does not destabilize public compatibility
or helper/oracle behavior.

Primary targets:

- `backend_riscv_prepared_edge_publication_test`
- `backend_prepared_lookup_helper_test`
- Any directly adjacent prepared-printer, route/debug, fallback, wrapper, exact
  output, or baseline assertions touched by Step 2.

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

Goal: leave lifecycle state ready for supervisor review and eventual
source-idea closure.

Actions:

- Summarize whether this row reduces the BO-family blocker from idea 265.
- Summarize which `memory_accesses` blockers remain.
- Record whether broader validation is recommended before closure.

Completion check:

- `todo.md` contains the proof summary, remaining blocker list, and exact
  validation evidence.
- The runbook can be reviewed without claiming broader demotion readiness than
  this one RISC-V same-consumer byte-offset drift row supports.
