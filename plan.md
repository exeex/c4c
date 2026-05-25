# Shared Call Plan Authority Runbook

Status: Active
Source Idea: ideas/open/01_shared_call_plan_authority.md

## Purpose

Move one meaningful AArch64 call-lowering decision family out of target-local
codegen and into the shared prepared call-plan layer.

Goal: make AArch64 call emission consume prepared call-plan facts for the
chosen family while target-local code remains responsible only for AArch64
machine instruction emission.

Core Rule: do not make a narrow testcase pass by adding named-case matching,
weakening tests, or rederiving placement decisions in AArch64 after claiming
they are shared.

## Read First

- `ideas/open/01_shared_call_plan_authority.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `tests/backend/mir/backend_call_boundary_effect_plan_test.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`

## Current Scope

- Keep the first pass focused on call-boundary move and preservation planning,
  because `PreparedCallPlan` and `plan_prepared_call_boundary_effects()` already
  model those facts and AArch64 currently has matching target-local lowering
  surfaces.
- Extend shared prepared facts only when the selected decision family cannot be
  expressed through the existing `PreparedCallPlan` API.
- Preserve AArch64 behavior while moving decision authority.

## Non-Goals

- Do not consolidate or delete AArch64 `calls*.cpp` files in this runbook.
- Do not start broad dispatch cleanup.
- Do not rewrite x86, RISC-V, assembly printing, or encoder behavior.
- Do not change C ABI semantics unless an existing bug is isolated and proven.
- Do not edit `ideas/open/02_aarch64_calls_emission_consolidation.md` or
  `ideas/open/03_dispatch_responsibility_reduction.md` as part of this plan.

## Working Model

- Shared planning authority lives under `src/backend/prealloc/`.
- AArch64 can translate prepared facts into machine nodes, but it should not
  reconstruct stack/register argument placement, preservation routes, or
  boundary move classification from raw BIR when the prepared call plan already
  has that information.
- Target ABI facts must stay parameterized. A shared helper may consume
  prepared banks, register names, placements, frame slots, and move bundles, but
  it must not hard-code AArch64-only register policy into generic planning.

## Execution Rules

- Make one coherent code-changing step at a time.
- For every code-changing step, run a fresh build or compile proof before
  acceptance.
- Pair narrow backend tests with the changed decision family; add broader
  runtime/backend proof when behavior crosses call ABI, byval, variadic, or
  preservation boundaries.
- Do not weaken tests, mark supported cases unsupported, or replace runtime
  assertions with expectation-only proof.
- If a required change belongs to emission consolidation or dispatch cleanup,
  record it in `todo.md` as a follow-up and keep this plan on shared call-plan
  authority.

## Ordered Steps

### Step 1: Audit the Existing Boundary-Move Authority

Goal: identify exactly which call-boundary decisions are already prepared and
which AArch64 helpers still make those decisions locally.

Primary target: `src/backend/prealloc/calls.hpp`,
`src/backend/prealloc/call_plans.cpp`,
`src/backend/mir/aarch64/codegen/calls_moves.cpp`, and
`src/backend/mir/aarch64/codegen/calls_preservation.cpp`.

Actions:

- Inspect `PreparedCallArgumentPlan`, `PreparedCallResultPlan`,
  `PreparedCallPreservedValue`, and `PreparedCallBoundaryEffectPlan`.
- Map AArch64 local classification or preservation decisions to prepared fields
  that already exist.
- Choose one first decision family to migrate, preferring the smallest family
  that proves AArch64 is consuming shared authority rather than rederiving it.
- Record any discovered gaps in `todo.md`; do not edit the source idea unless
  the durable scope is wrong.

Completion check:

- The next executor packet names one concrete decision family, the source
  prepared facts it will consume, and the AArch64 helpers it will stop using for
  that decision.

### Step 2: Strengthen Shared Prepared Facts for the Chosen Family

Goal: make the selected call decision available as an explicit shared prepared
fact when existing fields are insufficient.

Primary target: `src/backend/prealloc/calls.hpp`,
`src/backend/prealloc/call_plans.cpp`, and focused prealloc tests.

Actions:

- Add or adjust shared prepared fields or helper functions only for the chosen
  family.
- Keep target-specific ABI facts supplied through prepared placements, banks,
  register names, frame slots, or existing target profile inputs.
- Update prepared-printer or lookup support when needed to make the new fact
  inspectable.
- Add or update focused tests under `tests/backend/bir/` or
  `tests/backend/mir/` that prove the shared fact is present and ordered.

Completion check:

- The prepared layer exposes the chosen decision without requiring AArch64 to
  infer it from raw BIR shape.
- The focused prepared-layer proof passes with a fresh build or compile proof.

### Step 3: Consume the Shared Decision in AArch64 Emission

Goal: remove the equivalent AArch64-local decision path for the chosen family
and consume the prepared call-plan fact instead.

Primary target: `src/backend/mir/aarch64/codegen/calls_moves.cpp`,
`src/backend/mir/aarch64/codegen/calls_preservation.cpp`,
`src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`, and
`src/backend/mir/aarch64/codegen/calls.hpp`.

Actions:

- Route AArch64 lowering through the shared prepared call-plan fact.
- Keep AArch64-specific code limited to operand conversion and machine-node
  emission.
- Preserve diagnostics for missing or malformed prepared authority.
- Delete or narrow now-obsolete local decision helpers only when their behavior
  is covered by the shared path and tests.

Completion check:

- AArch64 emission for the selected family no longer reconstructs the moved
  decision from raw BIR or target-local compatibility matching.
- Focused AArch64 backend tests pass and no expectation contract is weakened.

### Step 4: Prove Behavior and Guard Against Overfit

Goal: prove the migrated family works generally enough to count as shared
call-plan authority progress.

Primary target: focused call-boundary, preservation, and relevant runtime or
backend route tests.

Actions:

- Run the narrow tests that cover the selected family before and after the
  code-changing packet when the supervisor requests regression logs.
- Include at least one nearby same-feature case when the migrated family has
  obvious variants such as register result, stack argument, byval aggregate, or
  preserved-value republication.
- Run a fresh build or broader backend subset after the first AArch64 consumer
  is changed.
- Escalate to reviewer scrutiny if the diff appears to add testcase-shaped
  matching or only proves a single named case.

Completion check:

- Build proof plus focused tests pass.
- The proof demonstrates migrated decision authority, not only unchanged output
  for one fixture.

### Step 5: Document Remaining Boundaries for Later Ideas

Goal: leave the next lifecycle ideas with clear handoff notes without starting
their work.

Primary target: `todo.md` and, only if durable source intent changed,
`ideas/open/01_shared_call_plan_authority.md`.

Actions:

- Summarize which call decision family moved to shared authority.
- List remaining AArch64-local call decisions that belong to future emission
  consolidation.
- Record blockers or intentionally deferred families in `todo.md`.
- Do not activate emission consolidation or dispatch reduction from inside this
  plan.

Completion check:

- The active runbook has established at least one shared call-plan authority
  migration and the remaining work is clear enough for the supervisor to either
  continue, request review, or close the source idea when acceptance criteria
  are met.
