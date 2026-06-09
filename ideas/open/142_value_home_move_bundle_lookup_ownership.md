# 142 Value-home and move-bundle lookup ownership

## Goal

Move value-home and move-bundle lookup ownership out of the broad
`prepared_lookups.hpp` facade and into value-location ownership while preserving
`PreparedFunctionLookups` as the central aggregate wiring point.

## Why This Exists

Idea 141 found that `PreparedValueHomeLookups` and
`PreparedMoveBundleLookups` are reverse indexes over
`PreparedValueLocationFunction` data, plus move-bundle key/accessor helpers.
Their public declarations have a clearer value-location owner than the residual
prepared lookup facade.

## In Scope

- Move `PreparedValueHomeLookups`, its builder, and indexed value-home/id
  helpers toward `src/backend/prealloc/value_locations.hpp`.
- Move `PreparedMoveBundleLookups`, move-bundle key builders, move-bundle
  builder, indexed move-bundle helpers, before-call/before-return ABI move
  helpers, and after-call result-lane binding lookup helpers toward
  value-location ownership.
- Move current-block entry publication status/query only if the route proves
  the implementation remains directly tied to value homes and block-entry
  publication collection.
- Keep `prepared_lookups.*` responsible for aggregate field projection and
  one-pass wiring.

## Out Of Scope

- Deleting `PreparedFunctionLookups`.
- Redesigning move-bundle semantics, call plans, return handling, or value-home
  construction.
- Moving target-specific AArch64 emission, ABI register spelling, scratch, or
  hazard policy into prealloc value-location code.
- Moving return-chain lookups.

## Acceptance Criteria

- Consumers include or call value-location-owned declarations for value-home and
  move-bundle lookup APIs where possible.
- `PreparedFunctionLookups` still constructs and exposes the relevant fields.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if shared prepared semantics change rather than
  declaration/include ownership.

## Reviewer Reject Signals

- The new owner is just a renamed prepared-lookup facade.
- Consumers are forced into local rescans instead of using reusable lookup
  facts.
- The aggregate builder is removed or multiple targets lose the one-pass
  preparation path.
- Return-chain helpers are swept into the move without a concrete owner.
