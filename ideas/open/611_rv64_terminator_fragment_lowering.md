# RV64 Terminator Fragment Lowering

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
Owning Layer: RV64/MIR consumer
Queue Order: 10
Prerequisites: prepared branch and operand authority must exist; branch stack-source residuals remain separate
Estimated Evidence Breadth: `70` unsupported terminator-fragment rows
Proof Surface: RV64 backend-object rows with prepared terminator fragments that lack target lowering

## Goal

Implement RV64 lowering for supported terminator fragments after prepared
branch operands and control-flow authority are present.

## Why This Exists

The current scan has `70` unsupported terminator-fragment rows. This is a
large target-consumer bucket, but it must not bypass branch stack-source
freshness contracts.

## In Scope

- RV64/MIR consumer lowering for supported terminator fragments.
- Guardrails for missing branch operands, stack-source freshness, or prepared
  control-flow facts.
- Proof across multiple terminator rows.

## Out Of Scope

- Prepared branch source publication.
- Move-bundle materialization, generic instruction fragments, ABI, runtime,
  expectations, unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple terminator-fragment rows progress through RV64 lowering.
- Rows whose first owner is branch stack-source freshness or prepared authority
  remain separated.
- The proof checks both progressing rows and still-rejected missing-authority
  rows.

## Reviewer Reject Signals

- Reject branch lowering that infers missing operands from final layout.
- Reject named-case-only terminator handling such as a single `src/20030910-1.c`
  shape.
- Reject combining branch-source authority publication with target consumer
  lowering.
- Reject expectation or unsupported-status downgrades.
- Reject helper renames that keep the same unsupported terminator fragment.
