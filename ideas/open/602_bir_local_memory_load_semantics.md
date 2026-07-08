# BIR Local-Memory Load Semantics

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: BIR semantic producer
Queue Order: 1
Prerequisites: current `470/1467` RV64 gcc_torture evidence; no RV64 target fixes required before this producer repair
Estimated Evidence Breadth: `82` local-memory load rows, with adjacent local-memory rows expected to become better classified after repair
Proof Surface: RV64 gcc_torture backend-object rows that stop in the `load local-memory semantic family`, plus nearby local-memory rows that must retain accurate first-owner diagnostics

## Goal

Repair BIR production of local-memory load semantics for ordinary C cases so
prepared and RV64 stages receive valid memory-use facts instead of stopping at
the semantic producer layer.

## Why This Exists

The current failure map identifies `82` local-memory load rows as the largest
single BIR producer diagnostic family. Target-local RV64 work cannot safely
consume loads that BIR never modeled.

## In Scope

- BIR load semantics for local frame memory and scalar values.
- Diagnostic-preserving handling for nearby rows whose address, store, GEP, or
  alloca authority is still missing.
- Same-family proof across multiple gcc_torture rows, not one named case.

## Out Of Scope

- Store, GEP, alloca, global initializer, RV64 consumer, ABI, runtime,
  expectation, unsupported-marker, allowlist, timeout, or accounting changes.
- Treating final assembly shape as proof of BIR memory semantics.

## Acceptance Criteria

- Multiple rows from the local-memory load diagnostic family progress past the
  original BIR producer stop.
- Rows whose first owner is store, GEP, alloca, prepared authority, or RV64
  consumption remain accurately classified instead of being forced through load
  repair.
- The proof uses the RV64 gcc_torture backend-object route or a narrower
  matching subset that exercises the same failing family.

## Reviewer Reject Signals

- Reject testcase-shaped matching against representative case names such as
  `src/20041124-1.c`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime
  comparison, or pass/fail accounting changes as progress.
- Reject RV64 target inference that papers over missing BIR load facts.
- Reject a broad local-memory rewrite that also changes store, GEP, alloca,
  ABI, or runtime behavior without separate ownership.
- Reject helper renames or diagnostic wording changes that leave the original
  load semantic failure mode intact.
