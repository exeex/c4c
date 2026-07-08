# BIR Local-Memory Alloca And Scalar Semantics

Status: Closed
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

## Closure Summary

Closed after the alloca-local repair moved all `11` current alloca
local-memory producer rows beyond the original BIR producer stop and preserved
adjacent owner boundaries for load, GEP, store, vector, scalar-cast,
unordered-compare, runtime, and success rows in the same-family breadth proof.

Proof included selected Step 3 coverage (`16/16`), naming-sensitive backend
coverage (`9/9`), full backend coverage (`346/346`), Step 4 same-family
breadth (`65/65`), and a close-time full-suite regression guard with no new
failures (`3375/3375` before and after).

The remaining `22` scalar/local-memory producer rows are follow-up scope, not
open acceptance criteria for this alloca-local repair closure:
`src/20020411-1.c`, `src/20041201-1.c`, `src/20070614-1.c`,
`src/complex-2.c`, `src/complex-5.c`, `src/complex-6.c`, `src/ffs-2.c`,
`src/20020227-1.c`, `src/20021118-2.c`, `src/921013-1.c`,
`src/961223-1.c`, `src/builtin-bitops-1.c`, `src/ffs-1.c`,
`src/ieee/acc1.c`, `src/ieee/acc2.c`, `src/pr42248.c`, `src/pr42691.c`,
`src/pr47538.c`, `src/pr56837.c`, `src/pr61725.c`,
`src/scal-to-vec1.c`, and `src/scal-to-vec2.c`.

## Reviewer Reject Signals

- Reject changes that special-case `src/20180921-1.c` or any single mixed row.
- Reject broad stack-frame or ABI lowering inside this BIR producer idea.
- Reject expectation rewrites or unsupported downgrades.
- Reject classifying unresolved prepared/RV64 authority failures as BIR alloca
  success.
- Reject helper-only reshuffles that leave alloca or mixed local-memory rows
  stopped at the same semantic failure.
