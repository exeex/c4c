# 232 Phase E3 Route 6 x86 scalar i32 argument-source route-debug follow-up

## Goal

Augment or replace one Route 6 x86 scalar `i32` argument-source route-debug row
with Route 6 scalar source agreement while retaining `ConsumedPlans`
compatibility and prepared call/debug authority.

## Why This Exists

The Route 6 closure proved that x86 scalar `i32` argument-source agreement can
support one route-debug row while keeping `ConsumedPlans` compatibility. Phase
E3 accepted this row for a later one-row implementation idea and retained x86
wrapper behavior, ABI/call wrapper policy, prepared call printer/debug,
direct-call/helper-oracle families, public fallback, and expected strings.

## In Scope

- One x86 scalar `i32` argument-source route-debug row.
- Positive Route 6 scalar argument-source agreement.
- Absent, invalid, duplicate/conflict, mismatch, and compatibility paths with
  `ConsumedPlans`.
- Stable ABI/call wrapper behavior, prepared call printer/debug output,
  direct-call/helper-oracle families, wrappers, and expected strings.

## Out Of Scope

- Broad x86 call wrapper migration, ABI/call policy movement, call lowering
  replacement, or prepared call printer/debug replacement.
- Direct-call/helper-oracle family replacement beyond the named route-debug
  row.
- Expected-string rewrites, baseline refreshes, helper renames, unsupported
  downgrades, timeout masking, or weakened fallback behavior.
- E4 wrapper migration, draft 155, E5, or broad prepared diagnostic/oracle
  retirement.

## Acceptance Criteria

- The positive route-debug row uses Route 6 scalar source agreement.
- Absent, invalid, duplicate/conflict, mismatch, and `ConsumedPlans`
  compatibility cases keep existing fallback behavior.
- x86 wrapper behavior, ABI/call wrapper policy, prepared call printer/debug,
  direct-call/helper-oracle families, and expected strings remain unchanged.
- Proof covers nearby same-feature scalar source cases, not only one known
  argument-source fixture.
- No broad call wrapper, ABI, baseline, or expected-string migration is required
  to claim progress.

## Reviewer Reject Signals

- The route-debug change is shaped around one x86 `i32` testcase instead of
  Route 6 scalar source agreement.
- It breaks or bypasses `ConsumedPlans` compatibility.
- It changes ABI/call wrapper behavior, prepared call printer/debug output,
  direct-call/helper-oracle families, wrappers, expected strings, or baselines.
- It weakens absent, invalid, duplicate/conflict, mismatch, or public fallback
  behavior.
- It turns the row into broad x86 call wrapper, ABI policy, or prepared call
  diagnostic migration.
