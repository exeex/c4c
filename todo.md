Status: Active
Source Idea Path: ideas/open/206_route7_comparison_provenance_diagnostic_oracle.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Adapter Completeness And Route Quality

# Current Packet

## Just Finished

Step 4 proved the selected materialized-condition Route 7 adapter is
provenance-only progress, not a fixture shortcut.

No code change was needed. Existing focused coverage now demonstrates valid
Route 7 materialized-condition provenance, selected machine-row wiring,
selected compare output preservation, absent-route fallback, invalid-reference
rejection, duplicate ambiguity rejection, condition-name mismatch fallback,
lhs provenance mismatch fallback, rhs provenance mismatch fallback, selected
global-load/selected-compare fallback preservation through the same-feature
materialized compare path, and unchanged adjacent fused compare branch output.

Route 7 remains local to the selected provenance lookup. Prepared/AArch64
authority still owns branch targets, final branch spelling, condition suffix
mapping, fused legality, emitted-register state, hazards, final instruction
order, and final assembler row construction.

## Suggested Next

Supervisor lifecycle decision: accept and commit this completed Route 7 slice,
then decide whether the active plan should close or move into a separate Route
7 follow-up boundary.

## Watchouts

- The selected row is intentionally limited to materialized-condition
  provenance; fused compare operand readers and Route 7 branch-condition
  records remain outside this plan.
- Do not treat selected fallback preservation as a migration target. It is
  proof that invalid or mismatched Route 7 facts fail closed to existing
  BIR/prepared behavior.
- Remaining non-goals: branch-policy cleanup, branch emission migration,
  fused-legality migration, compare/branch selection migration, expected-string
  weakening, unsupported downgrades, and helper-rename-only progress.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed. Proof log: `test_after.log`.
