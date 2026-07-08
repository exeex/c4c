# Scalar Compare Publication

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: prepared authority
Queue Order: 16
Prerequisites: branch and select publication boundaries must remain separate
Estimated Evidence Breadth: `3` scalar compare publication rows
Proof Surface: scalar compare publication authority diagnostics in the prepared layer

## Goal

Repair the small scalar compare publication authority gap if it remains after
higher-yield producer and consumer work has moved.

## Why This Exists

The current failure map contains only `3` scalar compare publication rows, so
this is an ordered tail idea rather than a first activation target.

## In Scope

- Prepared scalar compare publication authority.
- Diagnostics preserving branch, select, and RV64 instruction-fragment
  ownership.
- Proof across all currently visible scalar compare publication rows if
  practical.

## Out Of Scope

- General compare lowering, branch stack-source, select publication, RV64
  instruction fragments, ABI, runtime, expectations, unsupported markers,
  allowlists, timeouts, or accounting.

## Acceptance Criteria

- Scalar compare publication rows progress or are reclassified with concrete
  owner evidence.
- No unrelated branch, select, or RV64 instruction-fragment route is changed.
- Proof covers the complete current small family when possible.

## Reviewer Reject Signals

- Reject using this low-count idea to justify broad compare or branch rewrites.
- Reject named-case-only special handling.
- Reject expectation or unsupported-marker changes.
- Reject RV64 consumer changes claimed as prepared publication progress.
- Reject retaining the same scalar compare publication stop under renamed
  helpers.
