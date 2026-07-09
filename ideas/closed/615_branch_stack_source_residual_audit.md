# Branch Stack-Source Residual Audit And Repair

Status: Closed
Type: Implementation with required audit first
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/RV64 authority
Queue Order: 14
Prerequisites: audit closed branch stack-source contracts before implementation; do not combine with terminator lowering
Estimated Evidence Breadth: `10` branch stack-load authority/source freshness rows
Proof Surface: residual branch stack-source rows after ideas `590`, `592`, `593`, `594`, and `596`

## Goal

Audit the residual branch stack-source failures against the closed authority
contracts, then repair only the single owner proven by that audit.

## Why This Exists

The current failure map shows `7` branch stack-load authority rows and `3`
branch stack-load source freshness rows after a long branch stack-source
series. The next route must determine whether these are new roles,
aggregate-adjacent shapes, or missing consumption of existing authority.

## In Scope

- A short audit of the residual diagnostics against ideas `590`, `592`, `593`,
  `594`, and `596`.
- A narrow repair only if the audit identifies one owner.
- Proof across the residual branch stack-source family.

## Out Of Scope

- Generic terminator lowering.
- New branch architecture not supported by the audit.
- ABI, runtime, expectations, unsupported markers, allowlists, timeouts, or
  accounting changes.

## Acceptance Criteria

- The runbook records the audit result before code changes.
- Any implementation touches only the audited single-owner branch
  stack-source gap.
- Multiple residual rows progress or are reclassified with concrete evidence.

## Split-In From Idea 611

Idea `611` close-readiness classified `src/921124-1.c` as a mixed
join/select plus stack-backed condition/freshness residual. This idea owns only
the stack-source audit portion: a future runbook must first prove whether the
row's first owner is branch stack-source freshness instead of select
publication before making code changes.

## Closure Summary

Closed after the required Step 1 audit. The audit found no complete-authority
RV64 branch stack-source consumer repair to implement under this idea.

Current residual classification:

- `7` `unsupported_branch_stack_load_authority` rows have selected freshness
  evidence but stop first at `missing_stack_clobber_safety`:
  `src/20001017-1.c`, `src/loop-2e.c`, `src/pr39100.c`,
  `src/20000314-3.c`, `src/20140828-1.c`, `src/20080519-1.c`, and
  `src/20050125-1.c`.
- `3` `unsupported_branch_stack_load_source_freshness` rows stop at
  `missing_source_freshness_authority` / `no_candidate`:
  `src/930930-1.c`, `src/990127-1.c`, and `src/20060910-1.c`.
- `src/921124-1.c` currently stops at `unsupported_terminator_fragment`, not
  branch stack-source freshness or select publication.

Durable follow-up scope was split to:

- `ideas/open/635_prepared_branch_stack_clobber_safety_authority.md`
- `ideas/open/636_prepared_branch_stack_source_freshness_publication.md`

The `src/921124-1.c` split-in note remains covered by existing open
publication follow-ups (`616` and `617`) when refreshed diagnostics prove one
of those owners. No duplicate terminator idea was created from this audit.

## Reviewer Reject Signals

- Reject skipping the audit and applying another broad branch fix.
- Reject merging this route with RV64 terminator lowering.
- Reject named-case-only branch stack-load changes.
- Reject expectation or unsupported-marker downgrades.
- Reject retaining the same branch stack-source authority failure under a new
  helper name.
