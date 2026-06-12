# 215 Route 7 comparison provenance consumer

## Goal

Move exactly one comparison provenance consumer to Route 7 route/prepared
agreement without moving branch policy, suffix mapping, fused legality, or
final assembler ownership.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. Route 7 validates selected
comparison provenance, but prepared/AArch64 branch facts remain authoritative
for branch behavior and output.

## In Scope

- One named comparison provenance consumer.
- Route/prepared agreement for the same provenance.
- Prepared or generic BIR fallback for absent, invalid, duplicate/conflict, and
  mismatched Route 7 facts.
- Proof that machine-printer and branch-control expected strings remain
  unchanged.

## Out Of Scope

- Branch targets, condition suffix mapping, fused legality, hazards,
  emitted-register state, final instruction order, final assembler rows, branch
  policy, printer/debug contraction, or expected-string changes.
- Route 8 return-chain work or generic route-index facades.

## Acceptance Criteria

- Positive: route/prepared agreement supplies the same comparison provenance to
  the named consumer.
- Absent: missing Route 7 fact uses prepared or generic BIR fallback.
- Invalid: invalid reference fails closed.
- Duplicate/conflict: duplicate provenance facts keep prepared/AArch64
  behavior.
- Mismatch: route/prepared disagreement preserves prepared provenance.
- Fallback: branch policy, fused legality, suffixes, targets, and final
  assembler remain prepared/AArch64-owned.
- Printer/debug and expected-string proof shows machine-printer and
  branch-control expected strings remain unchanged.

## Reviewer Reject Signals

- Reject branch-policy, suffix-mapping, fused-legality, final-assembler, Route 8
  return-chain, or generic route-index migration.
- Reject expected-string rewrites, helper renames, baseline refreshes,
  unsupported downgrades, or weaker branch-control contracts.
- Reject testcase-shaped provenance handling without absent, invalid, conflict,
  mismatch, and fallback proof.
- Reject moving AArch64 branch policy into Route 7.
- Reject preserving the old prepared/generic fallback behavior under a new
  route-provenance facade.

## Closure Note

Closed on 2026-06-12 after active Route 7 execution completed all runbook
steps and acceptance review
`review/215_route7_comparison_provenance_acceptance_review.md` reported no
blocking findings. The accepted implementation moved exactly one consumer,
`lower_materialized_compare_condition_branch`, to Route 7 route/prepared/BIR
agreement while preserving prepared/AArch64 authority for branch behavior and
expected-string output.

Close proof used matching canonical logs for:

```sh
ctest --test-dir build -R '^(backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper|backend_aarch64_machine_printer)$' --output-on-failure
```

`test_before.log` and `test_after.log` both recorded 3/3 passing tests, and the
regression guard passed with non-decreasing allowance. No follow-up split is
needed for this source idea.
