# Route 6 call-use source adapter

## Goal

Migrate one selected call instruction and one argument or result source role to
Route 6 BIR records while retaining prepared call-plan fallback for ABI and
layout policy.

## Why This Exists

The Phase B2 readiness audit classifies Route 6 schema as sufficient for one
call-use source identity role. It also records that this is not a call-plan
replacement: ABI placement, wrappers, clobbers, outgoing stack, byval lanes,
variadic FPR counts, helper protocols, homes, move bundles, publication
routing policy, aggregate transport, and final call records remain outside BIR
schema.

This idea is the bounded follow-up for the Phase B2 Route 6 candidate:
selected call-use source adapter.

## In Scope

- Select exactly one call instruction and one argument or result source role.
- Use Route 6 records or indexes for source value/base value/name,
  materializable same-block producer, direct-global dependency,
  publication-source identity when semantic, call-site ordinal, and selected
  result-lane source identity when applicable.
- Preserve prepared call-plan fallback for ABI/layout-bound and
  policy-sensitive facts.
- Prove route/prepared source-id agreement, absent Route 6 fail-closed
  behavior, mismatched source-id fail-closed behavior, ambiguous or ABI-bound
  rejection, and unchanged AArch64/x86 output.

## Out Of Scope

- Replacing whole call plans or `PreparedCallPlanLookups`.
- Moving ABI placement, call wrapper kind, clobbers, outgoing stack sizing,
  byval lanes, variadic FPR counts, helper/carrier protocols, value homes,
  move bundles, aggregate transport layout, publication routing policy, or
  final call records into BIR schema.
- Deleting, privatizing, or renaming prepared call helpers.

## Acceptance Criteria

- The selected call-use reader obtains only Route 6 semantic source identity
  from BIR.
- Prepared call plans remain the authority for ABI/layout and policy-sensitive
  behavior.
- Proof covers source-id agreement, absent facts, mismatch, ambiguity,
  ABI-bound rejection, prepared fallback, and unchanged target output.

## Reviewer Reject Signals

- Reject a slice that claims Route 6 source identity replaces whole call plans
  or call-plan helper groups.
- Reject moving ABI placement, clobbers, stack sizing, byval lanes, variadic
  FPR counts, helper protocols, homes, move bundles, aggregate transport,
  publication routing policy, or final call records into BIR schema.
- Reject missing absent/mismatch/ambiguous/ABI-bound proof or direct-call/helper
  oracle message weakening.
- Reject output changes hidden by expected-string rewrites or unsupported
  downgrades.
- Reject an adapter that handles more than one selected call instruction and
  role before the first route/prepared proof is complete.

## Closure Note

Closed after implementing the selected AArch64 scalar call argument
source-producer Route 6 adapter. The adapter accepts Route 6 only for semantic
source identity when prepared source id/name/value, unique key, current-block
producer pointer validity, and scalar materialization availability agree;
otherwise it falls back to prepared call-plan lookup. Prepared call plans remain
authoritative for ABI, layout, helper protocol, policy, storage, movement, and
emitted output behavior. Close-time backend regression guard passed with 180
passing tests before and after.
