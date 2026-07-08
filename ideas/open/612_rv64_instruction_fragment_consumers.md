# RV64 Instruction Fragment Consumers

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: RV64/MIR consumer
Queue Order: 11
Prerequisites: move-bundle target materialization and terminator lowering should run first; sub-bucket before implementation if diagnostics are too mixed
Estimated Evidence Breadth: up to `97` binary/pointer, cast, and other instruction-fragment rows, excluding low-priority floating and select tails unless justified
Proof Surface: RV64/MIR unsupported instruction-fragment rows grouped by binary/pointer op, cast, and other fragment families

## Goal

Repair RV64/MIR consumer lowering for broad ordinary-C instruction fragments
after the move-bundle and terminator target routes are established.

## Why This Exists

The current RV64/MIR bucket includes `42` binary/pointer op rows, `32` other
instruction-fragment rows, and `23` cast rows. These are high enough to matter,
but the route must start with sub-bucketing rather than one mixed shortcut.

## In Scope

- RV64 consumer lowering for one or more clearly sub-bucketed instruction
  fragment families.
- Diagnostic-preserving rejection for fragments still missing producer or
  prepared authority.
- Proof across several rows in each implemented sub-family.

## Out Of Scope

- BIR semantic cast or binop production.
- Floating-only lanes unless they are proven ordinary-C leverage.
- Move-bundle, terminator, ABI, runtime, expectation, unsupported-marker,
  allowlist, timeout, or accounting changes.

## Acceptance Criteria

- The activated runbook first sub-buckets the instruction-fragment population
  if the current diagnostics are too mixed.
- Implemented sub-families progress across multiple rows.
- Non-implemented fragments keep accurate unsupported or owner diagnostics.

## Reviewer Reject Signals

- Reject a monolithic instruction-fragment patch that hides mixed ownership.
- Reject named-case-only lowering for a single binary, cast, or pointer op.
- Reject producer repair or expectation changes claimed as RV64 consumer
  progress.
- Reject including floating, vector, or library policy lanes without explicit
  evidence.
- Reject retaining the exact unsupported instruction fragment under renamed
  diagnostics.
