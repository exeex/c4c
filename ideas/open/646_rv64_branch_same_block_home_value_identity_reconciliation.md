# RV64 Branch Same-Block Home/Value Identity Reconciliation

Status: Open
Type: Implementation
Parent: `ideas/closed/636_prepared_branch_stack_source_freshness_publication.md`
Related:
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/636_prepared_branch_stack_source_freshness_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/RV64 branch stack-load home-value identity reconciliation
Queue Order: 46
Proof Surface: `src/990127-1.c` after the original branch stack-load source
freshness row is selected, where RV64 reaches a separate same-block RHS
`%lv.a` row with `home_value_mismatch` and
`source_freshness_status=missing_value`.

## Goal

Classify and repair the RV64 branch stack-load same-block home/value identity
boundary exposed by `src/990127-1.c` without reopening prepared branch
stack-source freshness publication.

## Why This Exists

Idea 636 moved the original `src/990127-1.c` `block_1` / `lhs` `%t6` row past
the `missing_source_freshness_authority` / no-candidate blocker by selecting
prepared branch stack-source freshness. The current first failure is a separate
same-block `role=rhs`, `value=%lv.a` row with
`authority_status=home_value_mismatch`,
`source_freshness_status=missing_value`, and zero freshness candidates. That
is a home/value identity reconciliation problem, not another prepared
freshness-publication packet.

## In Scope

- Refresh the `src/990127-1.c` same-block RHS `%lv.a` diagnostics and record
  the concrete prepared home, source value, branch block, and consumer point.
- Identify whether the mismatch is caused by missing producer identity,
  stale or absent home-value publication, an RV64 consumer key mismatch, or a
  more precise existing owner.
- Add producer or RV64 consumer support only when explicit prepared facts prove
  that the selected home and value are the same semantic branch operand at the
  consumer point.
- Preserve fail-closed diagnostics for missing value identity, mismatched
  homes, stale publication, ambiguous candidates, and unrelated branch operand
  shapes.

## Out Of Scope

- Prepared branch stack-source freshness publication closed by idea 636.
- RV64 terminator fragment lowering owned by idea 645.
- Clobber-safety authority, destination fan-in authority, parameter ABI/home
  admission, runtime behavior, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe confirms the current `src/990127-1.c` first owner and
  exact same-block RHS `%lv.a` home/value mismatch shape.
- At least one semantic same-block home/value identity shape advances past
  `home_value_mismatch`, or the row is reclassified to a more precise existing
  owner with current evidence.
- Negative proof keeps branch stack-load operands with missing, stale,
  ambiguous, or mismatched value/home identity rejected.

## Reviewer Reject Signals

- Reject treating `%lv.a`, `block_1`, or `src/990127-1.c` as a named-case
  shortcut instead of proving a semantic home/value identity rule.
- Reject RV64 inference of value identity from stack offsets, final assembly,
  source syntax, local names, or diagnostic strings.
- Reject reopening prepared branch source freshness when the original `%t6`
  freshness row is already selected.
- Reject merging terminator lowering, clobber-safety, destination fan-in, ABI,
  runtime, expectation, unsupported-marker, allowlist, timeout, or accounting
  work into this idea.
- Reject helper renames or diagnostic-only edits that leave
  `home_value_mismatch` / `source_freshness_status=missing_value` as the
  effective first owner.
