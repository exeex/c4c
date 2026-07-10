# Direct-Global Stack-Backed Pointer Branch Boundary

Status: Closed
Type: Implementation
Parent: `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
Related:
- `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/RV64 branch handling for direct-global stack-backed
pointer operands
Queue Order: 53
Proof Surface: `src/20000314-3.c` first pointer branch after idea 645.

## Goal

Classify and repair the branch boundary where a prepared pointer branch
compares a register-backed pointer with a direct-global stack-backed pointer
operand.

## Why This Exists

Idea 645 accepted only the fused pointer compare branch shape where the fused
condition and exactly one compared pointer operand are stack-backed with
explicit branch stack-load authority. Step 4 evidence showed
`src/20000314-3.c` still fails at `unsupported_terminator_fragment`: its first
pointer branch compares register `%p.varg0` with direct-global `@arg0`, and
prepared evidence publishes RHS branch-stack-load authority for stack-backed
`@arg0` only. That is a separate direct-global stack-backed pointer branch
boundary, not the condition-plus-one-stack-operand family closed by idea 645.

## In Scope

- Refresh diagnostics for the `src/20000314-3.c` first pointer branch and
  record the direct-global operand, selected branch stack-load authority, and
  register operand context.
- Determine whether existing prepared direct-global symbol authority is
  sufficient for branch consumption or whether a producer publication gap
  remains.
- Add RV64 or producer support only when direct-global identity, pointer
  freshness, and branch stack-load authority are explicit.
- Preserve precise rejection for missing direct-global identity, missing or
  ambiguous branch authority, stale pointer values, and unrelated terminator
  shapes.

## Out Of Scope

- Condition-plus-one-stack-operand fused branch lowering closed by idea 645.
- Stack-carried pointer source publication for `%t6` or `%t23`, owned by
  `ideas/open/653_stack_carried_pointer_source_publication_materialization.md`.
- Generic direct-global local-memory policy already closed by idea 631 unless
  refreshed evidence exposes a new branch-specific boundary.
- ABI, runtime policy, expectation, unsupported-marker, allowlist, timeout, or
  accounting changes.

## Acceptance Criteria

- A refreshed probe confirms the direct-global stack-backed pointer branch
  shape and current first owner for `src/20000314-3.c`.
- At least one semantic direct-global stack-backed pointer branch shape
  advances past `unsupported_terminator_fragment`, or the row is reclassified
  to a precise missing producer authority.
- Negative proof keeps missing, stale, ambiguous, or non-direct-global pointer
  branch operands rejected.

## Reviewer Reject Signals

- Reject named-case fixes for `src/20000314-3.c`, `%p.varg0`, or `@arg0`
  without a semantic direct-global stack-backed pointer branch rule.
- Reject accepting direct-global pointer operands from stack offsets, final
  assembly shape, source spelling, or diagnostic text instead of explicit
  prepared authority.
- Reject broad direct-global local-memory rewrites that do not prove the
  branch operand boundary first.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic-only edits that leave
  `unsupported_terminator_fragment` as the effective first owner.

## Completion Note

Closed after active runbook Steps 1-4 completed. The direct-global
stack-backed pointer branch boundary has focused semantic positive/negative
coverage; representative `src/20000314-3.c` object evidence now stops earlier
at the out-of-scope `unsupported_call_abi` owner before reaching this branch
boundary. Canonical backend subset logs were accepted with the documented
non-decreasing regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported 333 passed, 32 failed, 365 total both before and after, with no new
failing tests and result PASS.
