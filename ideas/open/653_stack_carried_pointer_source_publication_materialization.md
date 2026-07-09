# Stack-Carried Pointer Source Publication Materialization

Status: Open
Type: Implementation
Parent: `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
Related:
- `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
- `ideas/closed/636_prepared_branch_stack_source_freshness_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/RV64 stack-carried pointer source publication and
materialization
Queue Order: 52
Proof Surface: `src/20140828-1.c` `%t6` and `src/loop-2e.c` `%t23` after RV64
fused pointer branch terminator lowering advances past
`unsupported_terminator_fragment`.

## Goal

Define and repair the authority or materialization needed for stack-carried
pointer sources consumed by RV64 branch lowering after terminator admission is
no longer the first owner.

## Why This Exists

Idea 645 proved that the representative fused pointer branch shape can lower
through RV64 when explicit branch stack-load authority exists for the condition
and exactly one pointer operand. `src/20140828-1.c` and `src/loop-2e.c` now
emit object code but abort at runtime. Step 4 evidence classified the remaining
owner as stale or unmaterialized stack-carried pointer source publication or
materialization for `%t6` and `%t23`, not branch admission.

## In Scope

- Refresh object, prepared-BIR, and disassembly evidence for `%t6` in
  `src/20140828-1.c` and `%t23` in `src/loop-2e.c`.
- Identify the producer, selected stack slot, source value, consumer branch,
  and publication or materialization point for each stack-carried pointer.
- Add producer or RV64 consumer support only when explicit prepared facts prove
  the pointer source is fresh and materialized at the selected consumer point.
- Preserve fail-closed diagnostics for missing source publication, stale stack
  slots, ambiguous pointer sources, mismatched homes, and unrelated branch
  shapes.

## Out Of Scope

- RV64 terminator-fragment admission closed by idea 645.
- Publishing generic branch stack-source freshness already owned by prior
  branch freshness ideas unless refreshed evidence exposes a distinct producer
  gap.
- Direct-global stack-backed pointer branch operands owned by
  `ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md`.
- ABI, runtime policy, expectation, unsupported-marker, allowlist, timeout, or
  accounting changes.

## Acceptance Criteria

- A refreshed probe confirms whether `%t6` and `%t23` share a semantic
  stack-carried pointer publication or materialization boundary.
- At least one complete-authority stack-carried pointer source advances past
  the stale or unmaterialized runtime failure mode, or the route records the
  exact missing producer authority that blocks it.
- Negative proof keeps missing, stale, ambiguous, or mismatched pointer source
  publications fail-closed.

## Reviewer Reject Signals

- Reject named-case fixes for `src/20140828-1.c`, `src/loop-2e.c`, `%t6`, or
  `%t23` without a semantic stack-carried pointer source rule.
- Reject inferring pointer freshness or materialization from stack offsets,
  final assembly shape, source spelling, local names, or diagnostic strings.
- Reject reopening terminator-fragment lowering or weakening branch authority
  admission to make the runtime rows pass.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic-only edits that leave the same stale or
  unmaterialized stack-carried pointer source behind a new label.
