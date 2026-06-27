# Typed Prepared Call Argument Contracts Runbook: LocalFrameAddressMaterialization

Status: Active
Source Idea: ideas/open/414_typed_prepared_call_argument_contracts.md

## Purpose

Continue replacing optional-field call-argument source-selection routes with
typed prepared contracts. `FrameSlotAddress` and `FrameSlotValue` are already
migrated; this runbook migrates `LocalFrameAddressMaterialization`.

## Goal

`LocalFrameAddressMaterialization` routes must carry complete producer-owned
facts for local aggregate/byval pointer address publication, and target
consumers must stop reading those facts from ad hoc optional-field
combinations.

## Core Rule

Do not add optional fields to `PreparedCallArgumentSourceSelection`. This route
needs its own typed payload, producer-side verifier, and explicit target
consumption.

## Read First

- `ideas/open/414_typed_prepared_call_argument_contracts.md`
- `docs/prepared_fact_contracts/call_argument_contract_plan.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`

## Current Targets

- `PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`
- `PreparedCallArgumentSourceSelection` compatibility accessors in
  `src/backend/prealloc/calls.hpp`
- Producer selection in `src/backend/prealloc/call_plans.cpp`
- Aggregate transport interaction in `plan_prepared_aggregate_transport`
- Prepared verifier/report surfaces in
  `src/backend/prealloc/prepared_contract_verifier.*`
- RV64/AArch64 consumers that use local-frame address materialization for
  aggregate/byval pointer arguments

## Non-Goals

- Do not merge `LocalFrameAddressMaterialization` with `ByvalRegisterLane`.
- Do not alter ABI policy for byval aggregates or variadic calls.
- Do not rewrite completed `FrameSlotAddress` or `FrameSlotValue` route
  contracts except for shared helper cleanup.
- Do not weaken supported-path tests or expectation contracts.

## Working Model

- This route describes a local frame address derived from a prepared
  materialization, not a loaded value from the slot.
- Required facts include source/base value identity, selected pointer byte
  delta, materialization block/instruction, materialized frame-slot id,
  materialized byte offset, selected stack offset, source extent, and source
  alignment.
- Byval aggregate transport may consume the same storage facts, but byval lane
  register payloads remain a separate route.

## Execution Rules

- Track routine progress in `todo.md`; do not edit the source idea unless a
  separate initiative is discovered.
- Reuse the `FrameSlotAddress` and `FrameSlotValue` typed-route patterns where
  they apply, but keep this payload route-specific.
- Cite taxonomy rows `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`,
  `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`,
  `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, call-route portions of
  `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, and storage/home portions of
  `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`.
- Use audit rows `418-AUD-RV64-BYVAL-COHERENT-001`,
  `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`, and
  `418-AUD-A64-CALL-COHERENT-001` where the selected consumers match.
- For every code-changing step, run the supervisor-delegated build plus narrow
  tests. Run default CTest at the broad-validation checkpoint.

## Step 1: Confirm LocalFrameAddressMaterialization Boundary

Goal: identify required producer fields and consumer entry points for
`LocalFrameAddressMaterialization`.

Primary target: `call_plans.cpp` construction paths and aggregate transport
consumers.

Actions:

- Inspect register and computed-address producer branches.
- Record required materialization fields, pointer-delta fields, source
  identity, stack offset, extent, and alignment.
- Identify consumer sites in RV64/AArch64 and aggregate transport.
- Decide whether any byval coupling requires a separate idea before Step 2.

Completion check:

- Required and rejected fields are known.
- Consumer entry points are concrete enough for typed-payload migration.

## Step 2: Introduce Typed Local Materialization Payload and Bridge Accessor

Goal: expose a typed route while preserving staged compatibility.

Primary target: `src/backend/prealloc/calls.hpp`.

Actions:

- Add a typed `LocalFrameAddressMaterialization` payload.
- Add an `as_local_frame_address_materialization_route` compatibility query.
- Reject missing required materialization/source/range facts and cross-route
  payloads.
- Update the contract plan and focused route-query tests.

Completion check:

- The typed query exposes only coherent local-address materialization facts.
- The optional-field bag is not extended.

## Step 3: Add Producer-Side Verification

Goal: classify missing and contradictory local materialization facts before
target consumers attempt recovery.

Primary target: `src/backend/prealloc/prepared_contract_verifier.*`.

Actions:

- Add route statuses for missing source identity, base identity where required,
  pointer delta, materialization location, frame-slot id, byte offset, extent,
  alignment, and stack offset.
- Classify cross-route payloads and contradictory slot/offset/range facts as
  `producer_incoherent`.
- Add focused verifier tests.

Completion check:

- Missing facts report `producer_missing`.
- Contradictions report `producer_incoherent`.

## Step 4: Migrate Target Consumers

Goal: make selected consumers use the typed local materialization route.

Primary target: RV64/AArch64 aggregate/byval local-address lowering and
publication helpers.

Actions:

- Replace direct optional-bag reads with the typed query plus verifier checks.
- Keep `ByvalRegisterLane` payloads on their compatibility path.
- Do not infer stack offsets, source homes, or byval payload ranges from target
  ABI shape when typed facts are missing.
- Add or update focused consumer tests.

Completion check:

- Migrated consumers are exhaustive over the typed route.
- Missing typed facts fail closed before target recovery.

## Step 5: Broaden Validation and Decide the Next Route

Goal: prove the migration did not regress default CTest and choose the next
route boundary.

Actions:

- Run build plus default `ctest --test-dir build -j --output-on-failure`.
- Inspect any RV64 gcc_torture movement.
- Record the next candidate in `todo.md`.

Completion check:

- Default CTest does not regress.
- `todo.md` identifies the next route or requests lifecycle review.
