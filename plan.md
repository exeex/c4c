# Value-Home Move-Bundle Lookup Ownership Runbook

Status: Active
Source Idea: ideas/open/142_value_home_move_bundle_lookup_ownership.md

## Purpose

Move value-home and move-bundle lookup declarations out of the residual
`prepared_lookups.*` facade and into value-location ownership while preserving
`PreparedFunctionLookups` as the central aggregate wiring point.

## Goal

Make value-home lookup APIs, move-bundle lookup APIs, and related indexed
helpers available from value-location ownership without changing value-home
construction, move-bundle semantics, ABI call/return behavior, or target
emission policy.

## Core Rule

This is an ownership move only. Do not redesign move-bundle semantics, call
plans, return handling, value-home construction, target-specific AArch64
emission, ABI register spelling, scratch policy, or hazard policy.

## Read First

- `ideas/open/142_value_home_move_bundle_lookup_ownership.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- value-location implementation files under `src/backend/prealloc/`
- direct consumers found by searching the current target declarations

## Current Targets

- `PreparedValueHomeLookups`
- value-home lookup builder declarations
- indexed value-home and value-home id helper declarations
- `PreparedMoveBundleLookups`
- move-bundle key builders and move-bundle builder declarations
- indexed move-bundle helper declarations
- before-call and before-return ABI move helper declarations
- after-call result-lane binding lookup helper declarations
- current-block entry publication status/query only if inspection proves it is
  directly tied to value homes and block-entry publication collection

## Non-Goals

- Do not delete `PreparedFunctionLookups`.
- Do not redesign move-bundle semantics, call plans, return handling, or
  value-home construction.
- Do not move target-specific AArch64 emission, ABI register spelling, scratch,
  or hazard policy into prealloc value-location code.
- Do not move return-chain lookups.
- Do not perform broad residual include cleanup beyond consumers touched by
  this owner move.
- Do not fold call-argument, comparison, load-local, current-block join-routing,
  or residual include cleanup work into this route.

## Working Model

- `PreparedFunctionLookups` remains the aggregate that constructs and exposes
  prepared lookup fields.
- Value-home and move-bundle lookup APIs are reverse indexes over
  `PreparedValueLocationFunction` data plus move-bundle key and accessor
  helpers.
- The public owner should be `src/backend/prealloc/value_locations.hpp` unless
  inspection proves a narrower existing value-location owner is already in use.
- `prepared_lookups.*` should keep aggregate field projection and one-pass
  wiring, not public ownership of value-location lookup declarations.
- Consumers should include the value-location owner when they only need moved
  value-home or move-bundle declarations.

## Execution Rules

- Keep the move behavior-preserving.
- Preserve names, signatures, return behavior, and lookup semantics unless a
  compile-only owner move requires a mechanical include or namespace adjustment.
- Update includes only where the ownership move justifies it.
- Keep `PreparedFunctionLookups` construction, aggregate fields, and one-pass
  preparation wiring intact.
- Do not replace reusable lookup facts with local rescans.
- Avoid unrelated refactors, helper renames, formatting churn, and expectation
  rewrites.
- For code-changing steps, prove with `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if shared prepared semantics change instead of only
  declaration/include ownership.

## Step 1: Inspect Value-Home And Move-Bundle Surface

Goal: map the exact value-home and move-bundle lookup surface before moving
anything.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- value-location implementation files under `src/backend/prealloc/`
- direct consumers found by searching each target declaration and helper name

Actions:

- Locate declarations and definitions for value-home lookup structs, builders,
  and indexed helper functions.
- Locate declarations and definitions for move-bundle lookup structs, key
  builders, builders, indexed helpers, ABI move helpers, and after-call
  result-lane binding helpers.
- Search direct callers and include users for each target declaration.
- Inspect value-location source-file conventions to choose the narrowest public
  and implementation owner.
- Decide whether current-block entry publication status/query belongs in this
  route or should remain out of scope.
- Record the target list, consumers, chosen owners, and any deferred ambiguity
  in `todo.md` before editing code.

Completion check:

- `todo.md` records the declaration, definition, consumer, chosen-owner, and
  current-block publication decision map for the current targets.
- No implementation files are edited in this step.

## Step 2: Move Value-Home Lookup APIs To Value-Location Ownership

Goal: make value-location ownership the public home for value-home lookup
declarations.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- selected value-location implementation file
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Move `PreparedValueHomeLookups`, its builder declarations, and indexed
  value-home/id helper declarations to the selected value-location owner.
- Move definitions only when the selected implementation owner requires it;
  otherwise keep implementation location stable and include the new public
  owner.
- Keep `PreparedFunctionLookups` construction and field projection intact.
- Preserve reverse-index lookup semantics over `PreparedValueLocationFunction`.

Completion check:

- Value-home lookup declarations are no longer publicly owned by the residual
  `prepared_lookups.hpp` facade unless an aggregate dependency still requires
  compatibility.
- Implementations compile from the selected owner arrangement.
- The move does not introduce local rescans or behavior changes.

## Step 3: Move Move-Bundle Lookup APIs To Value-Location Ownership

Goal: make value-location ownership the public home for move-bundle lookup
declarations and helper APIs.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- selected value-location implementation file
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Move `PreparedMoveBundleLookups`, move-bundle key builders, move-bundle
  builder declarations, and indexed move-bundle helper declarations to the
  selected value-location owner.
- Move before-call and before-return ABI move helper declarations where they
  are value-location lookup helpers rather than target emission policy.
- Move after-call result-lane binding lookup helper declarations where they are
  value-location lookup helpers.
- Keep return-chain lookup helpers out of this route.
- Keep target-specific ABI register spelling, scratch, hazard, and final
  instruction emission policy out of shared prealloc ownership.

Completion check:

- Move-bundle lookup declarations are no longer publicly owned by the residual
  `prepared_lookups.hpp` facade unless an aggregate dependency still requires
  compatibility.
- Implementations compile from the selected owner arrangement.
- The move preserves existing call/return lookup behavior and does not absorb
  target-local emission policy.

## Step 4: Update Consumers And Narrow Includes

Goal: point consumers at the value-location owner without performing broad
residual include cleanup.

Actions:

- Update direct consumers to include `src/backend/prealloc/value_locations.hpp`
  or the selected value-location owner where they need moved declarations.
- Remove `prepared_lookups.hpp` includes only when a file no longer needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, return-chain
  helpers, or another residual prepared lookup API.
- Keep any remaining broad include where the file still names aggregate
  prepared lookup APIs.
- Avoid unrelated include cleanup deferred to idea 149.

Completion check:

- Consumers compile against the value-location owner for moved declarations.
- No consumer loses required aggregate prepared lookup declarations.
- The diff does not change lowering behavior.

## Step 5: Prove And Record The Ownership Move

Goal: validate the behavior-preserving ownership move and leave executor state
ready for supervisor review.

Actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- If validation fails, record the exact failure and the next narrowed fix in
  `todo.md`.
- If validation passes, record the proof commands and result in `todo.md`.
- Escalate to full CTest if the patch changes shared prepared semantics rather
  than only declaration/include ownership.

Completion check:

- Build proof and backend CTest proof are recorded in `todo.md`.
- `PreparedFunctionLookups` still constructs and exposes the relevant fields.
- No move-bundle, call-plan, return handling, value-home construction, AArch64
  emission, ABI register spelling, scratch, or hazard behavior changed.
- The active source idea acceptance criteria are satisfied or the remaining
  blocker is explicit.
