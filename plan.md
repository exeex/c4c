# Typed Prepared Call Argument Contracts Runbook

Status: Active
Source Idea: ideas/open/414_typed_prepared_call_argument_contracts.md

## Purpose

Replace the optional-field call-argument source-selection bag with typed
prepared call-argument route contracts.

## Goal

Migrated call-argument routes must carry complete producer-owned facts, and
RV64/AArch64 target consumers must stop recovering migrated argument homes from
formal order, type, callee shape, or target-local guesses.

## Core Rule

Do not add new optional-field combinations to
`PreparedCallArgumentSourceSelection`; each migrated route needs a typed payload,
producer-side completeness checking, and exhaustive target consumption.

## Read First

- `ideas/open/414_typed_prepared_call_argument_contracts.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`

## Current Targets

- `PreparedCallArgumentSourceSelection` and its existing producer/consumer
  accessors.
- Prepared verification for required route fields.
- RV64/AArch64 call-argument lowering paths that currently recover producer
  facts.
- `docs/prepared_fact_contracts/call_argument_contract_plan.md` as the contract
  plan that cites consumed taxonomy and audit rows.
- Focused tests for same-module subword scalar arguments and existing
  preservation/frame-slot routes.

## Non-Goals

- Do not change unrelated aggregate or variadic call ABI policy.
- Do not add target-specific inference when a typed route is absent.
- Do not rewrite all call lowering in one step.
- Do not use RV64 gcc_torture monotonic pass count as the acceptance gate.
- Do not rewrite expectations, allowlists, or supported-path contracts to claim
  progress.

## Working Model

- The existing source-selection bag is a compatibility boundary only while
  routes migrate.
- Each typed route should state exactly which producer facts are required and
  which target consumers may use them.
- Missing producer facts should fail before backend consumers try to infer
  argument homes.
- Temporary RV64 gcc_torture regressions are acceptable only when they are
  precise fail-closed contract diagnostics, not silent miscompiles.

## Execution Rules

- Keep source intent in the linked idea; track packet progress in `todo.md`.
- Prefer small route-by-route migrations with compatibility accessors over a
  broad call-lowering rewrite.
- Before touching target consumers, identify the taxonomy and audit rows being
  consumed, or record that no applicable audit row exists for the selected
  route.
- Reject testcase-shaped fixes for named torture files such as `divmod-1.c` or
  `20000403-1.c`.
- For every code-changing step, run a fresh build or compile proof plus the
  supervisor-delegated narrow tests. Escalate to default CTest when the blast
  radius crosses route or target boundaries.

## Step 1: Map Existing Call-Argument Routes

Goal: identify the concrete route families currently encoded in
`PreparedCallArgumentSourceSelection`.

Primary target: prepared call-argument selection producers, accessors, and
RV64/AArch64 consumers.

Actions:

- Inspect the current optional fields and `kind` values.
- Group existing behavior into typed route candidates, including prior
  preservation, frame-slot address/value, local address materialization, and
  byval lanes.
- Identify which routes need compatibility accessors for staged migration.
- Cross-check the selected first route against the taxonomy and target-consumer
  audit handoff documents.

Completion check:

- The first typed route to migrate is selected.
- Required producer-owned facts and current consumer entry points are known.
- `docs/prepared_fact_contracts/call_argument_contract_plan.md` can cite the
  consumed taxonomy/audit rows for that route or explicitly record no applicable
  row.

## Step 2: Introduce Typed Route Payloads and Bridge Accessors

Goal: represent the selected call-argument route with a typed payload while
preserving a staged compatibility path.

Primary target: prepared call-argument contract types and accessors.

Actions:

- Add a typed payload for the selected route with only fields valid for that
  route.
- Add compatibility accessors or bridge helpers so unmigrated code can coexist
  without growing the optional-field bag.
- Keep contradictory old optional-field combinations unrepresentable or
  rejected for the migrated route.
- Update the call-argument contract plan with the route scope and consumed
  taxonomy/audit rows.

Completion check:

- The migrated route can be constructed and queried through typed APIs.
- New code does not add optional fields to
  `PreparedCallArgumentSourceSelection`.
- The code builds with the compatibility bridge in place.

## Step 3: Add Producer-Side Verification

Goal: fail missing or contradictory producer facts before target consumers
attempt recovery.

Primary target: prepared verification and producer diagnostics for the selected
call-argument route.

Actions:

- Add verifier checks for every required field in the typed payload.
- Preserve or improve precise diagnostics for missing route facts.
- Add focused negative coverage for missing producer facts where existing tests
  allow it.
- Confirm fail-closed diagnostics do not mask silent target recovery.

Completion check:

- Missing required facts for the migrated route fail in prepared verification or
  producer diagnostics.
- Contradictory optional-field combinations for the migrated route are not
  accepted behind a renamed API.
- Narrow verifier/backend tests cover the new fail-closed behavior.

## Step 4: Migrate Target Consumers for the Selected Route

Goal: make RV64/AArch64 consumers exhaustively consume the typed route instead
of recovering migrated producer facts.

Primary target: RV64/AArch64 call-argument lowering paths for the migrated
route.

Actions:

- Replace target-local guesses about argument destinations, formal homes, stack
  offsets, or register banks with typed contract consumption.
- Remove or block fallback recovery for the migrated route when producer facts
  are absent.
- Keep unmigrated routes on the explicit compatibility path.
- Add focused tests for same-module subword scalar arguments and existing
  preservation/frame-slot behavior relevant to the migrated route.

Completion check:

- Migrated consumers are exhaustive over the typed route.
- Missing typed facts produce explicit diagnostics before lowering recovery.
- Focused tests pass without testcase-specific matching or expectation
  weakening.

## Step 5: Broaden Validation and Decide the Next Route

Goal: prove the migrated route did not regress normal CTest and prepare the next
route boundary without over-expanding this slice.

Primary target: default CTest plus representative RV64/AArch64 prepared/backend
coverage selected by the supervisor.

Actions:

- Run the delegated broad validation, normally including
  `ctest --test-dir build -j --output-on-failure`.
- Inspect any RV64 gcc_torture movement and classify regressions as precise
  fail-closed diagnostics or unacceptable behavior changes.
- Record the proof and remaining route candidates in `todo.md`.
- If the route migration exposes a separate initiative, write a new open idea
  through the lifecycle process instead of expanding this plan.

Completion check:

- Default CTest does not regress.
- Any RV64 gcc_torture regression is explained as a precise fail-closed
  contract diagnostic.
- `todo.md` identifies the next route candidate or notes that the current
  runbook needs lifecycle review.
