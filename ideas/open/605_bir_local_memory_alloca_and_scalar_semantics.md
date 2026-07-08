# BIR Local-Memory Alloca And Scalar Semantics

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: BIR semantic producer
Queue Order: 4
Prerequisites: local-memory load/store/GEP ownership must remain explicit; no RV64 consumer inference
Estimated Evidence Breadth: `12` alloca rows plus `26` scalar/local-memory mixed rows
Proof Surface: alloca local-memory and scalar/local-memory mixed semantic stops in the RV64 gcc_torture backend-object route

## Goal

Repair BIR production for alloca-backed local memory and scalar/local-memory
mixed semantics after the primary load, store, and GEP boundaries are clear.

## Why This Exists

The `38` combined rows in this follow-up are lower than load/store/GEP but
still producer-owned and likely to become important once the larger local
memory stops move.

## In Scope

- BIR alloca local-memory semantics.
- Scalar/local-memory mixed semantic cases where the first owner is the BIR
  producer.
- Preserving diagnostics for rows that belong to load, store, GEP, prepared
  authority, or RV64 consumers.

## Out Of Scope

- Global initializer bootstrap, ABI stack-frame layout, RV64 stack-frame
  consumption, runtime, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- More than one alloca or scalar/local-memory row progresses beyond the
  original BIR producer stop.
- The implementation does not weaken owner classification for load/store/GEP
  failures.
- Proof uses the RV64 gcc_torture backend-object route or an equivalent
  same-family subset.

## Reviewer Reject Signals

- Reject changes that special-case `src/20180921-1.c` or any single mixed row.
- Reject broad stack-frame or ABI lowering inside this BIR producer idea.
- Reject expectation rewrites or unsupported downgrades.
- Reject classifying unresolved prepared/RV64 authority failures as BIR alloca
  success.
- Reject helper-only reshuffles that leave alloca or mixed local-memory rows
  stopped at the same semantic failure.
