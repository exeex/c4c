# RV64 Prepared Global Value-Location Consumer

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/608_prepared_global_data_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: RV64 prepared-global consumer / value-location handling
Prerequisites: prepared/global authority facts must already prove global
identity, offset, width, extent, layout authority, and supported addressing.
Proof Surface: direct RV64 object-route probes and backend tests for prepared
global loads/stores whose producer facts are already complete.

## Goal

Teach the RV64 object route to consume already-supported prepared global-memory
facts when their value locations move through prepared register and frame-slot
homes.

## Why This Exists

Idea `608` closed the producer-authority gap for prepared global-memory rows.
The residual direct object failures for `src/pr36034-1.c` and `src/pr91137.c`
now occur downstream: prepared rows have global identity, offset, width, extent,
layout authority, and supported direct base-plus-offset addressing, but RV64
consumer code still rejects or mishandles the prepared value-location sequence.

## In Scope

- RV64 consumption of prepared global loads and stores when producer authority
  is already present.
- Value-location handling for prepared global load destinations that may be
  FPR/GPR registers or prepared frame-slot homes.
- Fail-closed diagnostics for unsupported prepared global value-location shapes.
- Focused proof for both floating and integer/aggregate representative paths,
  including `src/pr36034-1.c` and `src/pr91137.c` if they still reproduce this
  owner.

## Out Of Scope

- Prepared/global producer authority, selected object-data authority, direct
  global-symbol base-plus-offset authority, or mixed object-data slot authority.
- BIR initializer bootstrap, local memory, ABI, runtime/link behavior,
  expectations, unsupported markers, allowlists, timeouts, or accounting.
- Testcase-specific shortcuts for the named residual files.

## Acceptance Criteria

- More than one prepared-global consumer row progresses beyond the current RV64
  value-location stop, or reaches a defensible narrower downstream owner.
- Producer-authority checks remain fail-closed when required prepared global
  facts are absent, ambiguous, or unsupported.
- Object-route proof covers both the floating global-load frame-slot case from
  `src/pr36034-1.c` and the integer/aggregate load-store sequence from
  `src/pr91137.c`, unless refreshed diagnostics prove one belongs to a separate
  named owner.

## Reviewer Reject Signals

- Reject changes that weaken prepared/global authority gates to make RV64
  emission proceed without complete producer facts.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist changes,
  timeout/accounting edits, or diagnostic weakening claimed as consumer
  progress.
- Reject testcase-shaped branches for `src/pr36034-1.c`, `src/pr91137.c`, or
  specific global names from those files.
- Reject patches that move producer-authority reconstruction into RV64 instead
  of consuming existing prepared facts.
- Reject helper-only refactors that retain the same RV64 prepared-global
  value-location stop under a renamed diagnostic.
