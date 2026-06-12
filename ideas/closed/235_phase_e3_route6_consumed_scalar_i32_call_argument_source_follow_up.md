# 235 Phase E3 Route 6 consumed scalar i32 call-argument source follow-up

## Goal

Repair Route 6 consumed-plan lookup behavior so named scalar i32 call-argument
sources are available through `ConsumedPlans` when prepared call-plan authority
already records the source relationship.

## Why This Exists

During idea 234 Step 2, `backend_x86_handoff_boundary` advanced from the
compare-join stack-home assertion to:

```text
x86 Route 6 call-use boundary: scalar call argument source did not thread through ConsumedPlans
```

A dirty implementation attempted to backfill missing BIR call-argument source
relationships from prepared call-plan authority in
`src/backend/prealloc/call_plans.cpp`. The reviewer judged that repair
potentially real but out of scope for idea 234, where Route 6 is only a
regression guard.

## In Scope

- Route 6 scalar i32 call-argument source facts exposed through
  `find_consumed_scalar_i32_call_argument_source(...)` and related consumed
  call-argument plan lookup paths.
- Prepared call-plan authority for named i32 scalar arguments when it can
  provide source encoding, source value id, and source name without guessing.
- Focused fail-closed coverage for missing facts, mismatched prepared ids, and
  non-Route 6 or non-i32 argument cases.
- Preservation of existing `backend_x86_route_debug` row spelling and helper
  fallback behavior.

## Out Of Scope

- x86 compare-join stack-home handoff repair from idea 234.
- Prepared compare-join selected-value-chain metadata repair from idea 236.
- Broad ABI/call wrapper migration, prepared diagnostic/oracle retirement, or
  route-debug expected-string rewrites.
- Any named-testcase shortcut for the direct extern call fixture that does not
  generalize through prepared call-plan authority.

## Acceptance Criteria

- Route 6 consumed scalar i32 call-argument source lookup succeeds from
  semantic prepared call-plan facts for named scalar i32 arguments.
- Lookup remains fail-closed when Route 6 facts are absent, prepared ids
  disagree, source names are missing, or the argument is outside the supported
  scalar i32 path.
- `backend_prepared_lookup_helper` and `backend_x86_route_debug` remain green
  without expected-string or baseline rewrites.
- Any shared `ConsumedPlans`, prepared call/debug, or direct-call surface
  touched by the repair receives supervisor-selected broader validation.

## Parked State

Commit `ea2b3a133` landed the Route 6 named scalar i32 call-argument source
publication repair. The delegated proof advanced past the Route 6 assertion,
with `backend_prepared_lookup_helper` and `backend_x86_route_debug` green, and
then failed at the split-out idea 236 selected-value-chain assertion. Idea 235
remains open, not active, until supervisor-selected close validation can be run
without being blocked by idea 236.

## Closure Notes

- 2026-06-12: Closed after the split-out idea 236 blocker was repaired by
  commit `30093b137`.
- Commit `ea2b3a133` satisfied this idea's Route 6 source-publication scope:
  named scalar i32 call-argument source facts are published from prepared
  call-plan authority while fail-closed lookup behavior remains unchanged.
- Supervisor-selected broader prepared metadata validation in `test_after.log`
  passed `backend_prepared_lookup_helper`, `backend_x86_route_debug`, and
  `backend_x86_handoff_boundary`, plus the broader prepared/prealloc subset.
- Close-time regression guard was run against the available canonical logs and
  reported PASS with no new failing tests.

## Reviewer Reject Signals

- The change matches a direct extern call testcase name, label text, block
  shape, or assertion string instead of deriving facts from prepared call-plan
  authority.
- The slice weakens Route 6 fail-closed behavior or turns missing/mismatched
  source facts into best-effort guesses.
- The slice claims progress through route-debug expected-string rewrites,
  baseline refreshes, helper renames, or classification-only changes.
- The implementation changes x86 compare-join stack-home behavior or
  pointer-backed selected-value-chain metadata as part of this Route 6 repair.
- The exact old consumed-plan lookup failure remains present behind a renamed
  helper or alternate diagnostic path.
