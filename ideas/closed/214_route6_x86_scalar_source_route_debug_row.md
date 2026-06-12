# 214 Route 6 x86 scalar source route-debug row

## Goal

Replace exactly one x86 Route 6 scalar source agreement route-debug row while
keeping `ConsumedPlans`, prepared debug output, and expected strings
authoritative.

## Why This Exists

Phase D2 narrowed the conditional Route 6 wrapper/debug candidate to this one
Step 4 proof-shape row. It is not broad x86 call wrapper migration.

## In Scope

- One named x86 Route 6 scalar source agreement route-debug row.
- Route-native evidence that reports the same scalar source agreement as the
  prepared-backed path.
- Prepared fallback for absent, invalid, duplicate/conflict, mismatch, and
  `ConsumedPlans` compatibility behavior.
- No-change checks for x86 wrapper behavior.

## Out Of Scope

- Broad x86 call wrapper migration, call printer migration, direct-call/helper
  oracle family migration, or `ConsumedPlans` removal.
- ABI policy movement, public fallback removal, prepared API deletion, or
  expected-string redesign.

## Acceptance Criteria

- Positive: the route-debug row reports the same scalar source agreement as the
  prepared-backed path.
- Absent: no Route 6 fact keeps the prepared debug row.
- Invalid: invalid call-use reference keeps current route-debug behavior.
- Duplicate/conflict: ambiguous route source facts refuse route-native
  replacement.
- Mismatch: route/prepared disagreement reports or falls back exactly as today.
- Fallback: `ConsumedPlans` remains the compatibility boundary.
- Printer/debug and expected-string proof shows exact byte-stable route-debug
  text.
- Wrapper proof checks x86 wrapper behavior for no output change without
  wrapper contraction.

## Reviewer Reject Signals

- Reject broad x86 call wrapper, call printer, direct-call/helper-oracle, or
  `ConsumedPlans` migration.
- Reject expected-string rewrites, helper renames, baseline refreshes,
  unsupported downgrades, or weaker route-debug contracts.
- Reject testcase-shaped debug output for one scalar case without absent,
  invalid, conflict, mismatch, and fallback proof.
- Reject moving ABI or call wrapper policy into Route 6.
- Reject retaining the same prepared-backed limitation behind a route-debug
  relabel.

## Closure Note

Closed on 2026-06-12 after the Route 6 x86 scalar source route-debug row
runbook completed through acceptance handoff. The implemented slice replaced
only the selected scalar `i32` argument-source route-debug row, kept
`ConsumedPlans` as the compatibility boundary, preserved prepared fallback for
absent, invalid, duplicate/conflict, mismatch, and compatibility cases, and
left x86 wrapper behavior unchanged.

Closure evidence:

- Activation: `5820dd267`.
- Step 1 selected row proof: `1cc295baa`.
- Step 2 implementation: `f767324e9`.
- Step 3 route-debug and fallback proof: `32bbfa6ea`.
- Step 4 focused wrapper/backend proof: `11ac79d1a`.
- Acceptance review: `a8cc77a60`, recorded in
  `review/214_route6_x86_scalar_source_acceptance_review.md` with verdict
  acceptance-ready and no blockers.
- Close-time regression guard used existing matching `test_before.log` and
  `test_after.log` for
  `backend_x86_call_boundary_effect_ordering` plus
  `backend_prepared_lookup_helper`; it passed with non-decreasing results
  allowed for the lifecycle-only close.

Residual note: the aggregate `build-x86` wrapper target remained unavailable
because of unrelated stale joined-branch compile signatures. The acceptance
review judged this non-blocking because focused wrapper/backend proof passed
and the slice did not touch wrapper/emitter policy.
