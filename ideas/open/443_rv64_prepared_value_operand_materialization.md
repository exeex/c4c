# RV64 Prepared Value Operand Materialization

Status: Open
Type: Runtime correctness implementation idea
Parent: `ideas/open/423_rv64_runtime_mismatch_triage.md`
Owning Layer: RV64 lowering consumption of prepared BIR value graph
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`
Starting Representative: `src/pr81503.c`

## Goal

Repair RV64 lowering so binary, comparison, and stored scalar values consume
the intended prepared BIR producers instead of reusing stale or unrelated
operand registers.

## Why This Exists

Runtime triage for active idea 423 found that `src/pr81503.c` compiles, links,
and runs under c4c, but exits `255` while clang exits `0`. Semantic BIR and
prepared BIR preserve the intended value graph through the final store to
global `c`, including a prepared `store_source` whose producer is a binary
operation. The linked RV64 binary then materializes wrong operands in `foo` and
performs a later compare using registers that do not hold the loaded `c`
value.

This is an ordinary-C runtime correctness problem after prepared BIR has
producer authority, not an expected-output, runtime-runner, pass/fail
accounting, or unsupported-diagnostic problem.

## In Scope

- Start from the first-wrong-edge artifacts for `src/pr81503.c` in
  `docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md`.
- Trace RV64 lowering of prepared binary/comparison value producers and their
  consumers through stores, loads, and branch/compare operands.
- Ensure emitted RV64 instructions select registers or materialized values that
  correspond to the prepared producer graph.
- Add or update focused ordinary-C validation around nearby binary,
  comparison, zext, store, load, and compare cases before accepting the route.
- Preserve fail-closed behavior for rows that still lack producer authority or
  first-wrong-edge evidence.

## Out Of Scope

- Changing runtime expectations, output comparisons, allowlists,
  unsupported markers, or pass/fail accounting.
- Inferring producer facts in RV64 from source names, testcase names, function
  names, block indexes, raw source shape, or final exit code.
- Claiming ownership of abort or segfault runtime rows without separate
  first-wrong-edge evidence.
- Repairing frame-slot call-argument publication; that belongs to the separate
  follow-up from `src/20080506-2.c`.
- Repairing inline asm tied-output/result materialization; that is parked as a
  lower ordinary-C priority follow-up unless explicitly activated later.

## Acceptance Criteria

- `src/pr81503.c` no longer reaches the recorded wrong operand/materialization
  edge, and its c4c RV64 behavior matches clang for the focused reproduction.
- The fix is expressed as a general RV64 prepared-value consumption rule or
  materialization repair, not as a case-specific branch.
- Nearby ordinary-C value-producer and consumer tests demonstrate the same
  route is not limited to `src/pr81503.c`.
- The implementation keeps runtime mismatch accounting strict and does not
  weaken unsupported or expected-output contracts.
- Fresh build proof, focused representative proof, and supervisor-selected
  regression proof are available before closure.

## Reviewer Reject Signals

- Reject any diff whose main effect is to special-case `src/pr81503.c`, global
  `c`, functions named `foo` or `main`, a known block number, a specific
  register pair, or the final qemu exit code.
- Reject expectation rewrites, output rewrites, allowlist changes,
  unsupported-marker changes, runtime-comparison changes, or pass/fail
  accounting changes claimed as runtime progress.
- Reject a route that fixes only the target testcase while nearby ordinary-C
  binary/comparison/store/load consumers remain unsupported, untested, or
  unexamined.
- Reject reconstructing missing producer facts in RV64 from raw BIR/source
  shape when prepared BIR does not publish the needed value authority.
- Reject helper renames, classification-only changes, or log wording changes
  presented as capability repair.
- Reject broad RV64 rewrites that are not tied to prepared value-producer
  consumption or that hide the old wrong-operand failure behind a new
  abstraction name.
