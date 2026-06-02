Status: Active
Source Idea Path: ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Call-Boundary Record Shapes

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and canonical execution-state
skeleton for Step 1. No implementation work has started.

## Suggested Next

Start Step 1 by mapping the current aggregate register-lane publication shape,
the ordinary call-boundary record shapes that must remain unchanged, the
rejection paths to preserve, and the narrow proof command.

## Watchouts

- Keep this idea behavior-preserving.
- Do not move ABI construction, prepared-source authority, source/destination
  selection, scratch decisions, or preservation logic out of `calls.cpp`.
- Do not delete printer-side validation because instruction-record helpers
  validate an aggregate-lane view earlier.
- Use matching `test_before.log` and `test_after.log` once the accepted slice
  changes public instruction declarations, record schema, helper predicates, or
  printer dispatch contracts.

## Proof

Lifecycle-only activation. No build or test proof was required.
