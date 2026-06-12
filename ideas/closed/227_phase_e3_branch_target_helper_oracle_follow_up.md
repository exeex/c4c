# 227 Phase E3 branch-target helper-oracle follow-up

## Goal

Augment or replace one selected `find_prepared_control_flow_branch_target_labels(...)`
helper-oracle branch-target label row family with BIR structured successor
identity evidence when the private `BranchTargetIdentityPassContext` reader
agrees with prepared data.

## Why This Exists

Phase E1 proved agreement-gated BIR structured successor label identity for the
selected AArch64 conditional branch consumer. Phase E2 moved that selected read
behind `BranchTargetIdentityPassContext` and
`read_agreeing_bir_branch_target_labels(...)`. Phase E3 classified the matching
helper-oracle row family as ready for a later one-row implementation idea while
retaining prepared fallback and output authority.

## In Scope

- One selected branch-target helper-oracle row family tied to
  `find_prepared_control_flow_branch_target_labels(...)`.
- Positive BIR structured successor identity under prepared agreement.
- Fail-closed behavior for absent context, invalid ids, duplicate or conflict
  cases where applicable, mismatch, non-conditional BIR, and non-agreement.
- Proof that public prepared helper fallback, helper-oracle status/name/string
  behavior, printer/debug output, wrappers, expected strings, and nearby
  same-feature cases remain stable.

## Out Of Scope

- Broad control-flow helper, printer/debug, wrapper, branch-control, edge-copy,
  or emitted-output replacement.
- Branch spelling, branch scheduling, edge-copy scheduling, aggregate API, or
  target-policy migration.
- Baseline refreshes, expected-string rewrites, helper renames, unsupported
  downgrades, or weakened helper-oracle authority.
- Implementing E4 wrapper work, Route 8, draft 155, E5, or broad prepared
  diagnostic/oracle retirement.

## Acceptance Criteria

- The positive helper-oracle row can use BIR successor identity only after
  agreement with prepared data.
- Every absent, invalid, duplicate/conflict, mismatch, non-conditional, and
  non-agreement path keeps prepared fallback/oracle authority.
- Public helper fallback remains available and byte-stable.
- Tests or proof cover nearby same-feature branch-target cases so the change is
  not shaped only around one named case.
- No expected string, baseline, wrapper, printer/debug, branch-control, or
  emitted-output authority changes are required to claim progress.

## Closure Note

Closed after the active runbook completed Step 4. The selected materialized-bool
AArch64 conditional branch row has focused consumer proof for prepared
true/false successor retention and emitted branch payload stability under raw
BIR label drift, including invalid, mismatched, and conflicting structured
successor ids. Nearby same-feature proof remains covered by the focused
backend subset. Non-conditional BIR remains a helper-reader-level fallback case
because the selected consumer API rejects that terminator shape before the row
is reachable.

Closeout guard used the canonical focused logs:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with 3/3 tests before and after, 0 new failures.

## Reviewer Reject Signals

- The change special-cases one branch-target testcase or symbol name instead of
  reading the selected BIR successor identity through the private context.
- It weakens or removes prepared fallback for absent context, invalid ids,
  mismatch, non-conditional BIR, or non-agreement.
- It claims progress through helper renames, expected-string rewrites,
  unsupported downgrades, or baseline refreshes.
- It changes branch spelling, edge-copy scheduling, printer/debug output,
  wrapper output, or target/emitted-output policy while presenting this as a
  helper-oracle row slice.
- It leaves the old prepared-only success/failure mode behind a new helper name
  without using the E1/E2 BIR identity fact.
