# 234 Phase E3 x86 compare-join stack-home handoff follow-up

## Goal

Repair the x86 prepared-module consumer path so scalar-control-flow
compare-against-zero compare-join lanes with stack-backed parameter homes keep
following the authoritative prepared stack home through compare-join entry and
return.

## Why This Exists

Route 6 Step 3 for
`ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md`
confirmed the selected route-debug row behavior and passed
`backend_x86_route_debug` plus `backend_prepared_lookup_helper`. After the
compile-signature repair in `72907e31e` and the default build link regression
repair in `47557552b`, the broader `backend_x86_handoff_boundary` proof now
builds and fails semantically:

```text
scalar-control-flow compare-against-zero compare-join lane with stack-backed parameter home:
x86 prepared-module consumer stopped following the authoritative prepared stack home through compare-join entry and return
```

That failure is a nearby x86 handoff semantic issue, not evidence that the
selected Route 6 route-debug row should absorb compare-join stack-home
lowering work.

## In Scope

- The x86 prepared-module consumer path for compare-join entry and return when
  a scalar-control-flow compare-against-zero lane uses a stack-backed parameter
  home.
- The prepared stack-home authority used by `backend_x86_handoff_boundary`.
- Focused proof for the failing `backend_x86_handoff_boundary` semantic case
  and nearby same-feature handoff coverage.
- Preservation of the already-confirmed Route 6 route-debug row behavior.

## Out Of Scope

- Route 6 route-debug row spelling, expected strings, fallback matrix, or
  helper/debug contract changes from idea 232.
- Broad ABI/call wrapper migration, E4 wrapper migration, draft 155, E5, or
  prepared diagnostic/oracle retirement.
- Unsupported expectation downgrades, baseline refreshes, timeout masking, or
  weakening the handoff boundary assertion.
- Testcase-name shortcuts for the compare-join fixture.

## Acceptance Criteria

- `backend_x86_handoff_boundary` passes the compare-against-zero compare-join
  stack-backed parameter-home case without weakening its assertion.
- The implementation follows the authoritative prepared stack home through
  compare-join entry and return by semantic prepared/handoff state, not by
  fixture-specific matching.
- `backend_x86_route_debug` and `backend_prepared_lookup_helper` remain green
  with the Route 6 row behavior from idea 232 unchanged.
- Any touched shared x86 wrapper, module lowering, prepared dispatch,
  `ConsumedPlans`, MIR query, direct-call, helper-oracle, or prepared
  call/debug surfaces receive supervisor-selected broader validation.

## Reviewer Reject Signals

- The change matches the failing compare-join testcase name, label text, block
  shape, or assertion string instead of repairing prepared stack-home handoff
  semantics.
- The slice weakens, removes, or marks unsupported the
  `backend_x86_handoff_boundary` stack-backed parameter-home assertion.
- The slice claims progress through expected-string rewrites, baseline
  refreshes, helper renames, or classification-only changes.
- The Route 6 route-debug row from idea 232 changes spelling, fallback
  behavior, expected strings, wrappers, prepared call/debug output, or
  helper-oracle behavior as part of this repair.
- The exact old failure mode remains present behind a renamed helper or
  different diagnostic path.
- The implementation broadens into ABI/call wrapper migration or prepared
  diagnostic/oracle retirement beyond the compare-join stack-home handoff
  surface.
