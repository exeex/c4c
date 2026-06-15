# 272 Phase F5 riscv memory_accesses prepared-only fail-closed proof

## Closure Note

Closed after accepted code commit
`b5783084f4f409cdc05372293a4028350ad98588` proved the bounded RISC-V
same-consumer prepared-only `LoadLocal` row. The selected backend consumer now
fails closed with `UnsupportedSourceHome` when an internally coherent
`PreparedFunctionLookups::memory_accesses` row lacks agreeing Route 3 / Route
5 memory-source authority, while the positive agreement path remains stable at
`lw a1, 12(s2)`.

This closes only the source idea's bounded proof packet. It reduces the
prepared-only PO-family blocker from idea 265 for this one RISC-V same-consumer
row and does not claim x86 parity, stale-publication coverage, byte-offset
drift coverage, cross-publication mismatch coverage, or readiness to demote,
delete, privatize, wrap, hide, or rename `memory_accesses`,
`PreparedFunctionLookups`, or `PreparedBirModule`.

Closure evidence:

- Focused proof scope recorded in `test_before.log` and `todo.md`: 2/2 green
  for `backend_prepared_lookup_helper` and
  `backend_riscv_prepared_edge_publication`.
- Accepted full-suite baseline recorded in `test_baseline.log`: commit
  `b5783084f4f409cdc05372293a4028350ad98588`, `<full-suite>`, 3428/3428
  passing.
- Close-time regression guard run by plan-owner using the focused log with
  `--allow-non-decreasing-passed`: passed, 2/2 before and after, no new
  failures.

## Goal

Use the supported RISC-V dynamic stack-source `LoadLocal` fixture from idea
271 to prove one same-consumer prepared-only fail-closed row for
`PreparedFunctionLookups::memory_accesses`.

This is a bounded proof packet. It must not demote, delete, privatize, wrap,
hide, or rename `memory_accesses`.

## Why This Exists

Idea 270 tried to prove a prepared-only same-consumer row but closed as a
blocker map because no x86 or riscv proof surface directly read
`PreparedFunctionLookups::memory_accesses`.

Idea 271 then created the missing supported fixture route:

- target: RISC-V;
- shape: dynamic stack-source `LoadLocal`;
- consumer: a real backend consumer that directly reads
  `PreparedFunctionLookups::memory_accesses` through normal prepared lookup
  construction;
- compatibility output to preserve: `lw a1, 12(s2)`;
- route facts: matching Route 3/Route 5 authority facts are recorded as the
  starting point for follow-up proof.

This idea resumes the original proof intent from 270 using that supported
fixture. It should reduce the prepared-only PO-family blocker from idea 265
only for the selected RISC-V same-consumer row.

## Required First Step

Before editing code or tests, identify and record:

- the exact RISC-V backend consumer that reads
  `PreparedFunctionLookups::memory_accesses`;
- the concrete prepared memory row built by the idea 271 fixture;
- the matching Route 3 memory/source authority fact;
- the matching Route 5 publication/source owner, if relevant;
- the prepared-only mutation or fixture condition that removes or invalidates
  Route 3/Route 5 semantic agreement while leaving the prepared memory row
  internally coherent;
- the exact public compatibility output, status strings, helper/oracle rows,
  fallback behavior, and baseline-visible output that must remain stable.

If the prepared-only row still cannot be expressed without hand-built stale
prepared state, close with a blocker note and produce a narrower fixture or
testability idea instead of forcing synthetic state into this packet.

## In Scope

- One RISC-V same-consumer prepared-only row for the dynamic stack-source
  `LoadLocal` path from idea 271.
- Fail-closed behavior when the prepared memory row is present but matching
  Route 3/Route 5 semantic authority is absent or non-agreeing.
- Preservation of prepared lookup/status compatibility, helper/oracle names,
  fallback names, RISC-V exact output, route/debug output, prepared-printer
  output, and baseline behavior.
- Focused proof that the backend consumer does not treat the prepared-only row
  as route/BIR semantic agreement.

## Out Of Scope

- X86 memory-access proof.
- Stale-publication, byte-offset drift, or cross-publication mismatch proof
  unless it naturally falls out of the selected prepared-only row.
- Demotion, deletion, privatization, wrapper creation, accessor migration, or
  public API hiding for `memory_accesses`.
- Broad `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155
  retirement.
- Moving target-owned stack, storage, addressing, source-home, register,
  layout, wrapper, formatting, emission, instruction spelling, or exact output
  policy into BIR.
- Weakening expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baselines.

## Acceptance Criteria

- The selected RISC-V backend consumer and prepared memory row are named.
- The proof shows a prepared-only row fails closed when matching Route 3/Route
  5 authority is absent or non-agreeing.
- The positive agreement path from idea 271 remains stable.
- The compatibility output `lw a1, 12(s2)` and all adjacent status/fallback
  behavior remain byte-stable unless a separately approved source idea changes
  that contract.
- The closure note states exactly which PO-family blocker from idea 265 is
  reduced and which `memory_accesses` blockers remain.

## Suggested Proof Scope

Use a matching before/after proof around the RISC-V fixture and helper/oracle
surfaces touched by the packet. Candidate tests:

```bash
cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test
ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure
```

Adjust the build directory or target names only if the current repo setup
requires it, and record the exact command used in `todo.md`.

## Reviewer Reject Signals

- The proof does not reach the real RISC-V backend consumer that reads
  `PreparedFunctionLookups::memory_accesses`.
- The prepared-only row is hand-built stale state that normal prepared lookup
  construction would reject.
- The patch accepts prepared-only data as Route 3/Route 5 semantic agreement.
- The patch proves helper/oracle behavior only while claiming backend
  same-consumer coverage.
- The patch hides, demotes, deletes, privatizes, wraps, or renames
  `memory_accesses`.
- The patch weakens exact RISC-V output, status names, helper/oracle rows,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, or baselines.
- The patch claims whole `memory_accesses`, `PreparedFunctionLookups`, or
  `PreparedBirModule` demotion readiness from this one proof row.
