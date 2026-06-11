# 196 PreparedFunctionLookups ownership/readiness audit

## Goal

Build a field-by-field ownership and readiness map for
`PreparedFunctionLookups` after the Phase D selected-consumer route-view
migrations.

The map should classify each lookup group as BIR annotation, transient pass
context, target/prepared policy, compatibility adapter, retained oracle, or
blocked/unknown.

## Why This Exists

The pre-Phase-E readiness audit identifies `PreparedFunctionLookups` as a major
blocker to draft 155. The aggregate still projects call, address, move,
value-home, publication, select-chain/source-producer, memory, comparison, and
return-chain facts used by residual production, printer/debug, target-wrapper,
and oracle-test consumers.

True Phase E retirement analysis needs this aggregate map before it can reason
about `PreparedBirModule` field ownership or compatibility adapters.

## In Scope

- Inventory `PreparedFunctionLookups` lookup groups and their current
  consumers.
- Classify each lookup group by future ownership: BIR annotation, transient
  pass context, target/prepared policy, compatibility adapter, retained oracle,
  or blocked/unknown.
- Name residual production, printer/debug, target-wrapper, and oracle readers
  for each lookup group.
- Identify lookup groups already partially replaced by route views and the
  fallback/oracle surfaces that still keep them public.
- Produce prerequisites for any future aggregate contraction or compatibility
  adapter work.

## Out Of Scope

- Renaming `PreparedFunctionLookups` or splitting the type without first
  reducing coupling.
- Deleting lookup groups or privatizing accessors.
- Implementing route migrations found by the audit.
- Folding `PreparedBirModule` construction/mutation retirement into this
  aggregate audit.
- Opening draft 155 before the ownership map exists.

## Acceptance Criteria

- Every lookup group has an ownership/readiness classification and a named
  consumer set or a justified unknown status.
- The map distinguishes semantic route facts from target/prepared policy and
  retained oracle surfaces.
- Residual readers that block deletion or privatization are named explicitly.
- Compatibility-adapter needs are listed for lookup groups that can move toward
  route-native ownership.
- The output gives draft 155 enough evidence to decide which aggregate fields
  need detailed retirement analysis and which remain out of scope.

## Reviewer Reject Signals

- Claiming progress by renaming the aggregate or moving fields without reducing
  actual residual reader coupling.
- Deleting lookup groups while production, printer/debug, target-wrapper, or
  oracle readers still exist.
- Classifying target/prepared policy as BIR-owned merely because a selected
  route consumer moved.
- Weakening or deleting oracle coverage to justify a lookup-group contraction.
- Leaving the exact old coupling behind a new facade name.
- Collapsing this audit into broad `PreparedBirModule` retirement instead of
  producing the lookup-group map first.

## Completion Note

Closed after producing
`docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`.
The map inventories all seven `PreparedFunctionLookups` groups, names residual
production, printer/debug, target-wrapper, and oracle readers where present,
classifies ownership/readiness, and records one-boundary prerequisites for
future aggregate contraction or compatibility-adapter work.
