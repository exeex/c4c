# 187 Phase E Route 6 call-use source view consumer migration

## Goal

Switch one scalar call argument or result source reader to the Route 6 call-use
source view.

## Why This Exists

Phase D identified Route 6 source facts as usable for a bounded call consumer
migration while preserving call ABI, helper, and machine-record policy as
target-owned.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 call argument/result source-producer or publication-source
  materialization role class.
- A Route 6 view keyed by call instruction plus argument/result role and value
  role.
- Prepared call-plan, argument/result plan, publication-routing,
  call-argument source-producer, value-home, move-bundle, and call-boundary
  effect helpers as fallback/oracle surfaces.

## Out Of Scope

- ABI register/stack placement, variadic FPR counts, clobber/preserve sets,
  byval lanes, outgoing stack sizing, late-publication mechanics, helper
  resources, carrier protocols, or call record spelling.
- Deleting prepared call surfaces.
- Migrating multiple argument/result classes at once.

## Acceptance Criteria

- The selected role class reads Route 6 semantic source facts where available
  and preserves prepared fallback behavior.
- Tests cover direct source, publication source, missing source, and recursive
  operand fallback cases.
- Existing call-plan tests show ABI policy did not move into BIR.

## Reviewer Reject Signals

- Call ABI, helper, carrier, or record-spelling policy appears in the Route 6
  view.
- The implementation treats Route 6 absence as an error where prepared fallback
  is still required.
- Expectations are weakened or call-plan oracle tests are removed.
- The slice claims prepared call API contraction from one role-class migration.
- Route 6 facts are reconstructed with named-case matching.
