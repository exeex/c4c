# Typed Prepared Call Argument Contracts Runbook: FrameSlotValue

Status: Active
Source Idea: ideas/open/414_typed_prepared_call_argument_contracts.md

## Purpose

Continue replacing the optional-field call-argument source-selection bag with
typed prepared call-argument route contracts. The completed
`FrameSlotAddress` route is the pattern; this runbook migrates
`FrameSlotValue`.

## Goal

`FrameSlotValue` call-argument routes must carry complete producer-owned
source value/home/storage facts, and RV64/AArch64 consumers must stop reading
that route through ad hoc optional-field combinations.

## Core Rule

Do not add new optional fields to `PreparedCallArgumentSourceSelection`. The
`FrameSlotValue` migration must add a typed payload, producer-side completeness
checking, compatibility accessors, and exhaustive target consumption for the
selected route.

## Read First

- `ideas/open/414_typed_prepared_call_argument_contracts.md`
- `docs/prepared_fact_contracts/call_argument_contract_plan.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`

## Current Targets

- `PreparedCallArgumentSourceSelectionKind::FrameSlotValue`
- `PreparedCallArgumentSourceSelection` compatibility accessors in
  `src/backend/prealloc/calls.hpp`
- Producer selection in `src/backend/prealloc/call_plans.cpp`
- Prepared verifier/report surfaces in
  `src/backend/prealloc/prepared_contract_verifier.*`
- RV64/AArch64 consumers that currently read frame-slot value fields directly
- Focused tests in `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  and `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`

## Non-Goals

- Do not change ABI policy for aggregate, variadic, preservation, byval, or
  local address materialization routes.
- Do not rewrite the already migrated `FrameSlotAddress` route except for
  shared helper cleanup needed by `FrameSlotValue`.
- Do not add target-side fallback inference for missing source slots, stack
  offsets, value identity, extents, or alignments.
- Do not weaken expectations, supported-path contracts, or allowlists.

## Working Model

- The optional-field bag is a staged compatibility input only.
- `FrameSlotValue` requires selected source value identity, stack-slot source
  home identity, selected source slot, source stack byte offset, source extent,
  and source alignment.
- Missing required fields are producer-missing facts. Contradictory route/home
  payloads are producer-incoherent facts.
- Unmigrated routes stay on explicit compatibility paths until their own typed
  payloads are introduced.

## Execution Rules

- Keep source intent in the linked idea; track routine progress in `todo.md`.
- Reuse the `FrameSlotAddress` typed payload/verifier/test pattern where it
  applies, but do not merge the two routes into a vague shared bag.
- Cite the same taxonomy rows as the first route:
  `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`,
  `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`, call-route portions of
  `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, and storage/home portions of
  `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`.
- Use audit rows `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001` and
  `418-AUD-A64-CALL-COHERENT-001` first; cite RV64 call rows only where the
  selected RV64 path actually consumes `FrameSlotValue`.
- For every code-changing step, run the supervisor-delegated build plus narrow
  tests. Escalate to default CTest at the broad-validation checkpoint.

## Step 1: Confirm FrameSlotValue Route Boundary

Goal: identify the exact producer fields and consumer entry points for
`FrameSlotValue`.

Primary target: `FrameSlotValue` construction in `call_plans.cpp` and current
RV64/AArch64 consumers.

Actions:

- Inspect every `FrameSlotValue` producer path and record required fields.
- Identify which existing consumers use `source_value_id`,
  `source_value_name`, `source_home_kind`, `source_slot_id`,
  `source_stack_offset_bytes`, `source_size_bytes`, and
  `source_align_bytes`.
- Confirm whether any materialization, preservation, byval, source-base, or
  pointer-delta payload is valid for `FrameSlotValue`.
- Update `todo.md` with the selected compatibility accessor shape and proof
  expectations for Step 2.

Completion check:

- Required and rejected `FrameSlotValue` fields are known.
- The target consumer list is concrete enough for a bounded Step 2/3/4
  migration.

## Step 2: Introduce Typed FrameSlotValue Payload and Bridge Accessor

Goal: represent `FrameSlotValue` with a typed payload while preserving staged
compatibility with old producers.

Primary target: `src/backend/prealloc/calls.hpp`.

Actions:

- Add a typed `FrameSlotValue` route payload with only valid fields for that
  route.
- Add an `as_frame_slot_value_source_route` compatibility query.
- Reject missing required fields, non-stack source homes, cross-route payloads,
  and any contradictory optional-bag combinations.
- Update `docs/prepared_fact_contracts/call_argument_contract_plan.md` with the
  Step 2 `FrameSlotValue` scope.
- Add focused tests for valid and rejected typed route queries.

Completion check:

- `FrameSlotValue` can be constructed and queried through typed APIs.
- No optional fields are added to `PreparedCallArgumentSourceSelection`.
- The code builds with the compatibility bridge in place.

## Step 3: Add Producer-Side Verification

Goal: classify missing and contradictory `FrameSlotValue` producer facts before
target consumers attempt recovery.

Primary target: `src/backend/prealloc/prepared_contract_verifier.*`.

Actions:

- Add `FrameSlotValue` verifier statuses for missing route, value identity,
  stack-slot home identity, slot, stack offset, extent, and alignment.
- Classify cross-route payloads and wrong source-home kinds as
  `producer_incoherent`.
- Add focused verifier tests for coherent, missing, and incoherent reports.
- Update the contract plan with the verifier statuses.

Completion check:

- Missing required `FrameSlotValue` facts report `producer_missing`.
- Contradictory `FrameSlotValue` facts report `producer_incoherent`.
- The verifier test subset passes.

## Step 4: Migrate Target Consumers for FrameSlotValue

Goal: make selected RV64/AArch64 consumers use the typed route instead of
reading optional-bag fields directly.

Primary target: current frame-slot value call-argument lowering/publication
consumers.

Actions:

- Replace direct optional-bag reads for `FrameSlotValue` with
  `as_frame_slot_value_source_route` plus verifier checks.
- Preserve explicit compatibility for unmigrated routes only.
- Keep missing typed facts fail-closed; do not reconstruct stack offsets or
  source homes from raw argument order, ABI type, or target-local guesses.
- Add or update focused tests that prove coherent `FrameSlotValue` behavior and
  rejected missing/contradictory route facts.

Completion check:

- Migrated consumers are exhaustive over the typed `FrameSlotValue` route.
- Missing typed facts produce explicit producer-owned diagnostics or
  fail-closed behavior before lowering recovery.
- Focused RV64/AArch64 route tests pass without expectation weakening.

## Step 5: Broaden Validation and Decide the Next Route

Goal: prove the `FrameSlotValue` migration did not regress normal CTest and
choose the next route boundary.

Primary target: default CTest and route-candidate notes in `todo.md`.

Actions:

- Run the delegated broad validation, normally
  `ctest --test-dir build -j --output-on-failure` after a build.
- Inspect any RV64 gcc_torture movement and classify regressions as precise
  fail-closed diagnostics or unacceptable behavior changes.
- Record proof and remaining route candidates in `todo.md`.
- If the next route exposes a separate initiative, create a new open idea
  through the lifecycle process instead of expanding this runbook.

Completion check:

- Default CTest does not regress.
- Any RV64 gcc_torture regression is explained as a precise fail-closed
  contract diagnostic.
- `todo.md` identifies the next route candidate or notes that lifecycle review
  is required.
