# 256 Phase F3 prepared liveness authority blocker map

## Idea Type

analysis-only.

## Goal

Map why `PreparedBirModule::liveness` remains active prepared planning
authority before any demotion or private-pass-context work is proposed.

## Why This Exists

Phase F2 kept `liveness` separate from simple metadata. It may feed
prealloc/regalloc/helper-planning consumers and target register/storage policy,
so no private-pass-context demotion or deletion path is safe until those
consumers and replacement ownership are mapped.

## In Scope

- Inventory prealloc, regalloc, helper-planning, x86, riscv, and prepared
  printer/status consumers of `liveness`.
- Separate identity-only liveness metadata from allocation, move scheduling,
  storage/home, carrier/helper, target register, target storage, and output
  policy.
- Define fail-closed behavior for absent, invalid, stale, mismatch, fallback,
  and policy-sensitive liveness state.
- Record whether any one-reader implementation packet can be bounded later.

## Out Of Scope

- Demoting, deleting, or wrapping `liveness`.
- Moving register allocation, storage policy, move scheduling, or helper
  planning into BIR.
- Rewriting liveness-driven output, status, helper, or fallback expectations.
- Treating the route/invariants/completed_phases/notes gate as evidence for
  liveness.

## Acceptance Criteria

- The map identifies every liveness consumer bucket needed to decide future
  ownership.
- Any future implementation candidate is limited to one identity-only reader
  that excludes allocation, storage, helper planning, and target policy.
- If no such reader exists, `liveness` remains blocked public prepared
  planning authority.

## Reviewer Reject Signals

- Reject named-case shortcuts that inspect one liveness query or testcase and
  claim the whole field is private-pass-context ready.
- Reject unsupported expectation downgrades, weaker liveness fallback/status
  contracts, or helper-planning expectation rewrites without explicit approval.
- Reject helper renames, accessor renames, or classification-only notes
  claimed as liveness ownership progress.
- Reject broad prealloc, regalloc, storage, move scheduling, helper-planning,
  or target-policy rewrites outside the blocker map.
- Reject any route that retains the exact old liveness-planning failure mode
  behind a new route/BIR or private accessor name.

## Completion Note

Closed after the analysis-only runbook mapped liveness producer, direct
consumer, compatibility, and derived target buckets. No safe one-reader
implementation candidate remains: `PreparedBirModule::liveness` stays blocked
public prepared planning authority until a future idea supplies one exact
reader, one semantic fact, the retained prepared compatibility surface, and
full fail-closed proof for absent/skipped, stale, mismatch,
duplicate/conflict, unsupported, fallback, and derived printer/target behavior.
