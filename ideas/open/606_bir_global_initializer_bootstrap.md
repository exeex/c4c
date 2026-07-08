# BIR Global Initializer Bootstrap

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: BIR semantic producer
Queue Order: 5
Prerequisites: keep separate from prepared/global authority and RV64/global consumer ideas
Estimated Evidence Breadth: `38` global initializer bootstrap rows plus `2` string-pool bootstrap rows
Proof Surface: global initializer semantic bootstrap rows that stop before prepared/global authority

## Goal

Repair BIR global initializer bootstrap for ordinary aggregate and byte
initializer shapes that currently stop before prepared/global handoff.

## Why This Exists

The Step 3 plan separates global initializer bootstrap from local frame memory
because initializer bytes and local-memory facts have different authority and
proof surfaces.

## In Scope

- BIR semantic production for selected global initializer bytes and aggregate
  initializer shapes.
- Clear handoff boundary to later prepared/global authority work.
- Same-family proof across multiple initializer rows.

## Out Of Scope

- Prepared global object-data authority, RV64 global symbol emission, string
  library semantics, runtime/link behavior, expectations, unsupported markers,
  allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple global initializer bootstrap rows progress beyond the BIR producer
  stop.
- Prepared/global and RV64/global rows remain separate first-owner failures
  until their own ideas are activated.
- Proof does not depend on changing expected outputs or allowlist membership.

## Reviewer Reject Signals

- Reject named-case shortcuts such as matching only `src/20040302-1.c`.
- Reject mixing global initializer production with RV64 symbol emission.
- Reject treating string/library policy rows as initializer progress.
- Reject expectation or unsupported-marker changes.
- Reject leaving the same initializer bootstrap diagnostic behind a new
  abstraction name.
