# Prepared Branch Stack-Source Freshness Publication

Status: Open
Type: Implementation
Parent: `ideas/closed/615_branch_stack_source_residual_audit.md`
Related:
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `ideas/closed/615_branch_stack_source_residual_audit.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared branch stack-source freshness authority
Queue Order: 36
Prerequisites: branch-point freshness must be explicit before RV64 branch
stack-source consumers can accept fused stack-load branch operands
Estimated Evidence Breadth: `3` branch stack-load source freshness rows
Proof Surface: rows that still stop at
`missing_source_freshness_authority` / `no_candidate`

## Goal

Publish the prepared `BranchStackLoadSource` / `BranchStackSlot` freshness
authority for branch stack-load operands that currently have no selected
freshness candidate.

## Why This Exists

Idea 615 audited the branch stack-source residuals and found `3`
`unsupported_branch_stack_load_source_freshness` rows whose first owner is
`authority_status=missing_source_freshness_authority`,
`source_freshness_status=no_candidate`, and
`source_freshness_candidates=0`. The closed branch stack-source contracts
require producer-published branch-point freshness before RV64 consumption, so
these rows need prepared publication work rather than another RV64 consumer
fallback.

Representative rows:
`src/930930-1.c`, `src/990127-1.c`, and `src/20060910-1.c`.

## In Scope

- Refresh diagnostics for no-candidate branch stack-load source freshness rows.
- Publish explicit prepared branch stack-source freshness when the source slot,
  branch block, terminator point, and value relation are proven.
- Keep precise diagnostics for absent candidates, ambiguous candidates, stale
  freshness, and missing branch-point authority.
- Add focused producer/publication tests before relying on RV64 consumer proof.

## Out Of Scope

- Clobber-safety authority for rows that already have selected freshness; that
  belongs to idea 635.
- RV64 target-local inference of source freshness from stack layout, payload
  shape, or representative testcase names.
- Generic terminator lowering, select publication, compare publication, ABI,
  runtime, expectations, unsupported markers, allowlists, timeouts, or
  accounting.

## Acceptance Criteria

- A refreshed probe identifies a shared prepared freshness publication path for
  no-candidate rows.
- At least one no-candidate row gains explicit selected freshness and moves to
  the next legitimate owner, or all rows are reclassified with concrete current
  evidence.
- Negative proof keeps rows with missing source slot identity, ambiguous
  freshness, stale branch-point evidence, or separate terminator/select owners
  rejected.

## Reviewer Reject Signals

- Reject RV64 consumer changes that infer freshness instead of consuming
  prepared authority.
- Reject named-case-only publication for a single representative row.
- Reject merging clobber-safety, select publication, compare publication, or
  terminator lowering into this freshness publication idea.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave
  `source_freshness_status=no_candidate` as the effective first owner.
