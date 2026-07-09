# Prepared Branch Stack Clobber-Safety Authority

Status: Closed
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
Owning Layer: prepared branch stack-load authority
Queue Order: 35
Prerequisites: branch stack-load freshness must remain separate from
clobber-safety authority; RV64 must not infer safety from stack homes alone
Estimated Evidence Breadth: `7` branch stack-load authority rows
Proof Surface: rows with selected branch stack-source freshness that still stop
at `missing_stack_clobber_safety`

## Goal

Publish or validate the prepared clobber-safety authority needed for branch
stack-load operands whose source freshness is already selected.

## Why This Exists

Idea 615 audited the remaining branch stack-source rows and found `7`
`unsupported_branch_stack_load_authority` cases with
`source_freshness_status=selected` and one freshness candidate, but their first
owner is `authority_status=missing_stack_clobber_safety`. These rows are not
ready for an RV64 consumer-only fix because the closed branch contracts require
explicit safety authority at the branch point.

Representative rows:
`src/20001017-1.c`, `src/loop-2e.c`, `src/pr39100.c`,
`src/20000314-3.c`, `src/20140828-1.c`, `src/20080519-1.c`, and
`src/20050125-1.c`.

## In Scope

- Refresh diagnostics for selected branch stack-source rows that fail only on
  clobber-safety authority.
- Define or publish explicit prepared evidence that the selected stack source
  is safe from intervening clobbers at the branch use point.
- Preserve the distinction between selected source freshness and independent
  clobber-safety authority.
- Add focused tests that prove missing, contradictory, or stale safety evidence
  remains fail-closed.

## Out Of Scope

- RV64 fallback inference from frame homes, source filenames, final assembly, or
  stack offsets.
- Branch stack-source freshness publication for no-candidate rows; that belongs
  to idea 636.
- Generic terminator lowering, select publication, ABI, runtime, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe identifies a shared clobber-safety authority gap rather
  than one named testcase shape.
- Multiple rows with already-selected branch stack-source freshness either move
  past `missing_stack_clobber_safety` or are reclassified to a more precise
  owner with current evidence.
- Negative proof keeps rows without selected freshness, without clobber-safety
  evidence, or with contradictory clobber evidence outside this route.

## Reviewer Reject Signals

- Reject target-local acceptance of branch stack loads based only on stack
  homes, offsets, or apparent no-clobber assembly.
- Reject named-case-only fixes for any representative source file listed here.
- Reject merging no-candidate freshness publication, select publication, or
  terminator lowering into this clobber-safety idea.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave
  `missing_stack_clobber_safety` as the effective first owner.

## Closure Notes

Closed after Step 6 residual classification. The seven representative rows no
longer report `missing_stack_clobber_safety` as their first owner after the
prepared branch clobber-safety and call-preservation authority work.

Current residual owners:
- `src/20001017-1.c`: `unsupported_call_abi`
- `src/loop-2e.c`, `src/pr39100.c`, `src/20000314-3.c`,
  `src/20140828-1.c`, `src/20080519-1.c`, and `src/20050125-1.c`:
  `unsupported_terminator_fragment`

Follow-up ownership is split to:
- `ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md`
- `ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md`
