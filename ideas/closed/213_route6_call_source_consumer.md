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

## Closure Note

Closed after acceptance review in
`review/213_route6_call_source_consumer_acceptance_review.md`.

The selected consumer was the primary non-`result_lanes_only` AArch64
call-result source-register publication. The implementation supplies Route 6
evidence only under route/prepared agreement and preserves prepared fallback
for missing, invalid, duplicate/conflicting, and mismatched facts. ABI,
wrapper, aggregate transport, final call records, printer behavior, storage,
movement, and emitted behavior remain prepared or target-owned.

Regression guard used the available matching `test_before.log` and
`test_after.log` four-test backend proof. The strict-increase checker reported
no new failures but rejected equal pass count; the lifecycle-only close gate
was accepted with the documented non-decreasing allowance:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Residual note: x86 route-debug byte-stability was unavailable in this build
because `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`; no x86 proof was claimed, and
the acceptance review treated this as a non-blocking proof limitation because
the slice did not touch x86 files or expected strings.
