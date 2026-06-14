# 254 Phase F3 prepared compatibility fail-closed proof matrix

## Idea Type

compatibility-retention proof.

## Goal

Create a reusable proof matrix for prepared compatibility surfaces that must
remain stable while future route/BIR semantic facts move underneath them.

## Why This Exists

Phase F2 found that fallback behavior, unsupported fail-closed behavior,
helper/oracle status names, x86 route-debug output, wrapper assembly/output,
and target-policy-sensitive output remain compatibility authority for every
exit path. Future implementation packets need a concrete proof matrix instead
of rewriting expectations ad hoc.

## In Scope

- List required cases for missing, invalid, duplicate/conflict, mismatch,
  unsupported, prepared-only, fallback, and policy-sensitive behavior.
- Cover helper/oracle statuses, fallback names, route-debug names, prepared
  printer/debug text, wrapper output, `ConsumedPlans`, source-memory statuses,
  publication statuses, and exact target output rows that future packets must
  preserve.
- Mark which rows are common across fact families and which are specific to
  calls, memory, edge publications, source producers, names, control flow, or
  store-source publications.

## Out Of Scope

- Implementing adapters or demotions.
- Refreshing baselines or expected strings.
- Treating compatibility preservation as semantic ownership transfer.
- Moving target policy into BIR.

## Acceptance Criteria

- The result gives future reviewers a concrete checklist for rejecting
  expectation weakening or old-failure retention.
- Each row identifies the public prepared compatibility surface that remains
  authoritative and the route/BIR agreement or fail-closed behavior required
  before a future implementation can be accepted.
- The matrix does not authorize any field deletion, privatization, or broad
  prepared retirement.

## Reviewer Reject Signals

- Reject named-case shortcuts that prove only one expected string or one
  baseline while claiming broad compatibility retention.
- Reject unsupported expectation downgrades, weakened status names, weaker
  fallback behavior, or baseline rewrites without explicit approval.
- Reject helper renames, status renames, or classification-only preservation
  claims as compatibility proof.
- Reject broad helper/oracle, wrapper, route-debug, target-output, or prepared
  printer rewrites outside the proof matrix.
- Reject any route that keeps the exact old failure mode but changes only the
  compatibility surface name.

## Completion Note

Closed after Step 4 validated the compatibility-retention proof matrix as a
future reviewer checklist. The matrix is documentation/analysis-only and does
not approve implementation, adapters, demotions, baseline rewrites, field
deletion, privatization, broad prepared retirement, public-surface weakening,
or target-policy migration into route/BIR facts.

Future packets must treat the open proof gaps recorded in the completed
matrix as citation/test obligations. Those gaps are not permission to relax
public prepared compatibility contracts, helper/status spelling, fallback
behavior, target-policy-sensitive output, or fail-closed requirements.
