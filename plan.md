# Phase E Route 4 Publication View Consumer Migration Runbook

Status: Active
Source Idea: ideas/open/182_phase_e_route4_publication_view_consumer_migration.md

## Purpose

Turn the Route 4 publication-view follow-up from Phase D into one narrow,
testable AArch64 consumer migration.

## Goal

Switch one current/block-entry publication reader to Route 4-backed publication
identity through a narrow typed view or validated facade, while keeping prepared
publication helpers available as oracle and fallback surfaces.

## Core Rule

Migrate one real publication consumer through semantic Route 4 data. Do not
delete prepared publication APIs, move target policy into BIR, or use
testcase-shaped matching.

## Read First

- `ideas/open/182_phase_e_route4_publication_view_consumer_migration.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- AArch64 publication helpers and current/block-entry publication readers
- Route 4 publication availability records and existing fail-closed facade code

## Current Targets

- One AArch64 value-publication or dispatch-publication reader that asks for
  current/block-entry publication identity.
- Route 4 publication availability records, typed references, or a narrow
  validated facade around them.
- Existing prepared current-block and block-entry publication helpers as
  comparison and fallback answers.
- Route/prepared equivalence tests for present, absent, and mismatched
  reference cases.

## Non-Goals

- Do not delete or narrow prepared publication helpers.
- Do not move value homes, storage availability, move planning, publication
  record construction, or block-order emission into BIR.
- Do not migrate prepared printer, debug, x86 wrapper, or oracle-test consumers.
- Do not turn Route 4 into a generic lowering-plan aggregate.

## Working Model

- Route 4 is the semantic source for the selected consumer's publication
  identity when validation succeeds.
- Prepared publication helpers remain public and available for oracle,
  fallback, and comparison behavior.
- The migrated boundary must fail closed when Route 4 data is absent,
  mismatched, or insufficient.

## Execution Rules

- Keep the slice limited to one selected consumer and its immediate typed-view
  or facade boundary.
- Preserve existing oracle and publication tests; do not weaken expectations to
  make the migration pass.
- Prefer small, local helper extraction over broad publication pipeline
  reshaping.
- For code-changing steps, run a fresh build or compile proof plus the narrow
  route/prepared publication tests selected by the supervisor.

## Ordered Steps

### Step 1: Select Consumer And Baseline Behavior

Goal: identify the exact AArch64 publication reader to migrate and record its
current prepared-helper behavior.

Concrete actions:

- Inspect AArch64 current/block-entry publication readers and pick one
  value-publication or dispatch-publication consumer.
- Identify the prepared publication helper calls it currently relies on.
- Identify the Route 4 records or facade inputs that should provide the same
  publication identity.
- Record the selected files, functions, and narrow proof command in `todo.md`.

Completion check:

- `todo.md` names the selected consumer, prepared fallback/oracle surfaces,
  Route 4 source surfaces, and proof subset for the implementation packet.

### Step 2: Define The Narrow Route 4 Boundary

Goal: expose only the publication identity needed by the selected consumer
through typed Route 4 data or a fail-closed facade.

Concrete actions:

- Add or reuse a narrow typed reference/facade for the selected publication
  identity.
- Validate present, absent, and mismatched reference states at the boundary.
- Keep target storage, move planning, publication construction, and block-order
  policy outside BIR-owned code.

Completion check:

- The boundary compiles, is local to Route 4 publication identity, and still
  allows prepared helpers to answer oracle/fallback queries.

### Step 3: Migrate The Selected Consumer

Goal: make the chosen consumer read publication identity through the Route 4
boundary when valid, without changing unrelated consumers.

Concrete actions:

- Replace the selected direct prepared-helper read with the Route 4 typed view
  or validated facade.
- Preserve prepared fallback/oracle behavior for invalid or unavailable Route 4
  data.
- Avoid broad helper renames or generic facade expansion.

Completion check:

- The selected consumer no longer depends on direct prepared publication
  identity for the validated Route 4 path, and nearby consumers are not
  silently migrated.

### Step 4: Prove Route/Prepared Equivalence

Goal: cover the migrated boundary with focused tests for matching and
fail-closed behavior.

Concrete actions:

- Add or update tests proving Route 4 and prepared helper equivalence for the
  selected present-publication case.
- Add absent and mismatched-reference coverage that proves fail-closed
  validation.
- Keep existing oracle and publication tests at least as strong as before.

Completion check:

- Narrow route/prepared tests pass and show present, absent, and mismatched
  behavior for the migrated boundary.

### Step 5: Validate And Hand Off

Goal: make the slice ready for supervisor review without claiming broader API
contraction.

Concrete actions:

- Run the delegated build or compile proof and narrow publication subset.
- Run broader validation if the supervisor determines the slice affects more
  than the selected consumer boundary.
- Update `todo.md` with changed files, proof results, residual risks, and a
  clear Suggested Next.

Completion check:

- Fresh validation is recorded, prepared publication APIs remain public, and
  the route does not claim deletion or broad consumer migration from this one
  slice.
