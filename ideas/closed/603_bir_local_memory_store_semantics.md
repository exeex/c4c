# BIR Local-Memory Store Semantics

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: BIR semantic producer
Queue Order: 2
Prerequisites: preserve the load-semantics boundary from `602`; no RV64 consumer work is prerequisite
Estimated Evidence Breadth: `56` local-memory store rows
Proof Surface: RV64 gcc_torture backend-object rows that stop in local-memory store semantic production

## Goal

Repair BIR production of local-memory store semantics without mixing store
source authority, address formation, and target materialization into one
route.

## Why This Exists

The current scan has `56` store-owned local-memory producer stops. These rows
block prepared and RV64 stages before any target lowering decision is valid.

## In Scope

- BIR store semantics for ordinary local frame writes.
- Store source and destination validation at the BIR producer boundary.
- Proof that multiple store-family rows progress while non-store rows keep
  their correct owner.

## Out Of Scope

- Load, GEP, alloca, global data, RV64 target, ABI, runtime, expectation,
  unsupported-marker, allowlist, timeout, or accounting changes.

## Acceptance Criteria

- Same-family local-memory store rows progress beyond the current BIR producer
  stop.
- Load, GEP, alloca, and prepared authority failures are not reclassified as
  stores solely to make the proof pass.
- The proof includes more than one store-shaped row from the current scan.

## Closure Summary

Closed after the Step 10 lifecycle review. Accepted implementation under this
idea repaired BIR local-memory store production for full-width `f128` local
literal source values, loaded global pointer-field authority, dynamic local
pointer-array stores, dynamic local aggregate-array element copies, and
single-lane SSA vector stores into local vector lane slots.

Refreshed evidence showed the source acceptance criteria satisfied: multiple
store-shaped rows progressed beyond the old local-memory store producer stop,
guard owners preserved their boundaries, and no broad remaining multi-row
in-scope local-memory store producer subfamily was visible after adjacent owner
families were separated.

Known leftover: `src/931102-2.c` remains a singleton in-scope local
union/overlap store limitation. Adjacent follow-up work remains outside this
idea, including pointer/address authority, function-label/local pointer-array
authority, downstream scalar/local-memory classification, global-data routes,
and aggregate/static global store handoff under
`ideas/open/619_bir_aggregate_global_store_handoff.md`.

## Reviewer Reject Signals

- Reject named-case shortcuts such as matching only `src/20010605-2.c`.
- Reject weakening diagnostics or changing expected outcomes without semantic
  store production.
- Reject RV64-side materialization used to compensate for missing BIR store
  facts.
- Reject merging this route with local-memory GEP, alloca, ABI, runtime, or
  global data work.
- Reject code movement that retains the exact unsupported store family under a
  new helper name.
