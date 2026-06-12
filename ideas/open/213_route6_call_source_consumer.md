# 213 Route 6 call argument or result source consumer

## Goal

Move exactly one call argument or result source consumer to Route 6
route/prepared agreement while retaining prepared call-plan authority.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. Route 6 can identify a
selected call-use source, but prepared call plans remain authoritative for ABI,
layout, helper, storage, movement, and final call behavior.

## In Scope

- One named call argument or result source consumer.
- Route/prepared agreement for the same source.
- Prepared fallback for absent, invalid, duplicate/conflict, and mismatched
  Route 6 facts.
- Proof that prepared call printer and x86 route-debug output remain unchanged.

## Out Of Scope

- ABI placement, wrapper kind, clobbers, outgoing stack sizing, byval lanes,
  variadic FPR counts, helper/carrier protocols, value homes, move bundles,
  publication routing, aggregate transport, final call records, storage,
  movement, or emitted output.
- Broad x86 call wrapper migration, public fallback removal, or call-plan API
  deletion.

## Acceptance Criteria

- Positive: route/prepared agreement supplies the same argument or result
  source for the named consumer.
- Absent: no Route 6 fact uses prepared call-plan source.
- Invalid: invalid call-use reference fails closed.
- Duplicate/conflict: conflicting route source facts do not override prepared
  call plans.
- Mismatch: route/prepared disagreement preserves prepared source.
- Fallback: ABI, wrapper kind, aggregate transport, and final call records
  remain prepared/target-owned.
- Printer/debug, wrapper, and expected-string proof shows prepared call printer
  and x86 route-debug output remain unchanged.

## Reviewer Reject Signals

- Reject broad call-plan, ABI, wrapper, or x86 migration.
- Reject moving ABI or final call-record policy into Route 6.
- Reject expectation rewrites, baseline refreshes, helper renames, unsupported
  downgrades, or weaker test contracts.
- Reject testcase-shaped call-use handling without general fail-closed
  route/prepared agreement for the named consumer.
- Reject preserving the old prepared-only behavior behind a route-named access
  path.
