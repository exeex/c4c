# RV64 Branch Residual Terminator Fragment Lowering

Status: Closed
Type: Implementation
Parent: `ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md`
Related:
- `ideas/closed/611_rv64_terminator_fragment_lowering.md`
- `ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md`
- `ideas/closed/636_prepared_branch_stack_source_freshness_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`
Owning Layer: RV64 object-route terminator fragment lowering
Queue Order: 45
Proof Surface: branch-stack residual rows that now stop at
`unsupported_terminator_fragment` after clobber-safety or freshness authority
is no longer first owner.

## Goal

Repair or precisely classify the remaining RV64 object-route terminator
fragment failures for branch-stack residual rows without weakening branch
freshness or clobber-safety contracts.

## Why This Exists

Idea 635 closed the clobber-safety authority gap for its representative rows.
Six residual rows now share `unsupported_terminator_fragment` as first owner:
`src/loop-2e.c`, `src/pr39100.c`, `src/20000314-3.c`, `src/20140828-1.c`,
`src/20080519-1.c`, and `src/20050125-1.c`. Idea 636 later routed two more
branch-stack source freshness residuals here after prepared freshness was
selected: `src/930930-1.c` and `src/20060910-1.c`. The active clobber-safety
and freshness routes should not absorb generic terminator lowering.

## In Scope

- Refresh diagnostics for the residual rows and identify their concrete
  BIR terminator shapes.
- Compare those shapes against existing RV64 terminator-fragment support and
  closed idea 611 evidence.
- Add RV64 object-route terminator lowering only for semantic terminator shapes
  with explicit branch-source freshness and clobber-safety evidence where
  required.
- Preserve precise rejection for unsupported branch operands, missing
  freshness, missing clobber-safety, or unrelated terminator forms.

## Out Of Scope

- Publishing branch stack-source freshness or clobber-safety authority.
- Parameter ABI/home admission for `src/20001017-1.c`.
- Select publication, compare publication, runtime behavior, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe groups the residual rows by concrete terminator shape and first
  unsupported lowering boundary.
- At least one supported same-family terminator shape advances through semantic
  RV64 lowering, or all rows are reclassified to more precise owners with
  current evidence.
- Negative proof keeps branch operands without required freshness and
  clobber-safety authority rejected.

## Reviewer Reject Signals

- Reject named-case fixes for any of the representative source files.
- Reject accepting branch stack operands by stack offset, frame home, final
  assembly shape, or apparent no-clobber source shape.
- Reject merging ABI parameter-home work, branch freshness publication, or
  clobber-safety publication into this terminator-lowering idea.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave
  `unsupported_terminator_fragment` as the effective first owner.

## Closure Notes

Closed after RV64 object-route lowering admitted a prepared fused pointer
compare branch shape where the fused condition and exactly one pointer operand
are stack-backed, using explicit `branch_stack_load_authority` for
`role=condition` plus the selected `role=lhs` or `role=rhs` operand authority.

Focused coverage includes condition-plus-RHS and condition-plus-LHS positive
paths, missing or ambiguous condition authority rejection, and existing
operand-authority negatives. `src/20140828-1.c` and `src/loop-2e.c` now advance
past `unsupported_terminator_fragment` and fail later at runtime. The remaining
owner for those rows is stack-carried pointer source publication or
materialization, not terminator-fragment admission.

`src/20000314-3.c` remains at `unsupported_terminator_fragment` for a separate
direct-global stack-backed pointer branch boundary outside the accepted
condition-plus-one-stack-operand path.

Follow-up ideas created:

- `ideas/open/653_stack_carried_pointer_source_publication_materialization.md`
- `ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md`
