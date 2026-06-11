# Phase E Route 1 Producer/Constant View Consumer Migration

Status: Active
Source Idea: ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md

## Purpose

Move one AArch64 scalar value materialization or publication consumer onto a
narrow Route 1 producer/constant semantic view while keeping prepared target
policy and helper surfaces available.

Goal: prove one selected consumer can read same-block scalar producer and
integer-constant facts from Route 1 where available, with prepared helpers kept
as fallback and oracle surfaces.

## Core Rule

Route 1 may expose semantic producer and integer-constant facts only. Do not
move value homes, storage encodings, register choice, rematerialization spelling,
move generation, emitted machine records, or other target policy into BIR.

## Read First

- `ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- Existing Route 1 oracle tests and the closed Route 1 migration history named
  from the Phase D document.

## Current Targets / Scope

- One AArch64 scalar producer, integer-constant, or value-publication subpath
  that currently reads `find_prepared_same_block_scalar_producer(...)`,
  `evaluate_prepared_same_block_integer_constant(...)`, scalar operand helpers,
  or prepared value-publication helpers.
- A consumer-keyed Route 1 view or validated facade keyed by the consuming BIR
  value or instruction-local operand reference.
- Route/prepared comparison coverage for same-block producer, constant,
  no-producer, non-constant, and cross-block cases.

## Non-Goals

- Do not delete or hide prepared producer, constant, scalar operand, or
  value-publication APIs.
- Do not migrate unrelated scalar materialization paths.
- Do not claim route-wide API contraction from one selected consumer.
- Do not replace semantic Route 1 lookup with broad BIR rescans, name matching,
  or testcase-shaped shortcuts.

## Working Model

- The selected consumer should prefer Route 1 facts when a typed Route 1 answer
  is present and valid for that consumer.
- Prepared helper answers remain public and should still provide fallback or
  oracle comparison behavior where Route 1 is absent or intentionally out of
  scope.
- Target materialization and publication record construction stay in the
  AArch64/prepared layer.

## Execution Rules

- Keep each code slice behavior-preserving except for the intended source of
  semantic producer/constant facts.
- Add or extend tests before claiming a prepared helper read has moved.
- Preserve existing Route 1 oracle coverage; do not weaken expected output to
  make the selected consumer pass.
- If the selected consumer reveals a distinct initiative, record it separately
  instead of expanding this plan.

## Ordered Steps

### Step 1: Select and document the Route 1 consumer

Goal: choose one bounded AArch64 consumer that can be migrated without pulling
target policy into Route 1.

Primary targets:

- AArch64 scalar value materialization or publication code that currently asks
  prepared producer or integer-constant helpers for semantic facts.
- Nearby tests that already exercise producer/constant materialization or
  publication behavior.

Actions:

- Inspect the candidate consumer paths named in the Phase D document.
- Pick one consumer subpath with a clear route/prepared comparison story.
- Record the selected consumer, current prepared reads, intended Route 1 key,
  and proof subset in `todo.md` before implementation starts.
- Reject candidates that require changing storage homes, register selection,
  move bundles, emitted records, or materialization spelling.

Completion check:

- `todo.md` names the selected consumer, the prepared helper reads to compare
  against, the intended Route 1 lookup key, and the narrow proof command the
  executor should run.

### Step 2: Build the narrow Route 1 consumer view

Goal: expose the selected consumer's producer/constant semantic facts through a
typed Route 1 lookup boundary.

Primary targets:

- Existing Route 1 index, annotation, or facade code.
- The smallest helper boundary needed by the selected AArch64 consumer.

Actions:

- Add or reuse a consumer-keyed Route 1 view for same-block scalar producer and
  integer-constant facts.
- Make absence, no-producer, non-constant, and cross-block results explicit.
- Keep prepared helper calls available as comparison or fallback answers.
- Avoid copying prepared value-home, storage, register, move, or machine-record
  data into the Route 1 view.

Completion check:

- The selected consumer can query Route 1 facts through a typed boundary, and
  existing prepared helper APIs remain available and behaviorally unchanged.

### Step 3: Migrate the selected consumer route-first

Goal: make the selected consumer read Route 1 facts where available while
preserving prepared fallback behavior.

Primary targets:

- The selected AArch64 consumer from Step 1.
- Any local adapter needed to translate the consumer's operand or value
  reference into the Step 2 Route 1 lookup key.

Actions:

- Replace the selected semantic prepared read with route-first lookup.
- Use prepared fallback only where Route 1 is absent or intentionally cannot
  answer.
- Keep target-specific materialization, publication, storage, and record
  formation in existing AArch64/prepared code.
- Keep the change limited to the selected consumer subpath.

Completion check:

- Existing behavior is preserved for the selected consumer across route-present,
  route-absent, non-constant, no-producer, and cross-block cases.

### Step 4: Prove route/prepared equivalence

Goal: verify the selected consumer migration without weakening oracle coverage.

Primary targets:

- Existing Route 1 oracle tests.
- New or updated tests for the selected consumer.

Actions:

- Add tests comparing route-first answers with prepared helper answers for
  same-block producer, integer constant, no-producer, non-constant, and
  cross-block cases.
- Run the narrow AArch64 subset selected in Step 1 plus existing Route 1 oracle
  coverage.
- Run build or compile proof required by the supervisor for the touched code.

Completion check:

- The delegated proof command is green, the selected consumer tests cover the
  acceptance cases, and no Route 1 or prepared oracle tests were weakened.

### Step 5: Handoff and contraction guard

Goal: leave the route in a state that supports future consumer migrations
without claiming unsupported prepared API contraction.

Actions:

- Update `todo.md` with the completed consumer, proof results, residual
  prepared fallback/oracle surfaces, and any follow-up ideas discovered.
- Confirm prepared producer, constant, scalar operand, and value-publication
  helper APIs are still public.
- Confirm the slice does not claim route-wide migration or prepared aggregate
  contraction.

Completion check:

- `todo.md` contains the final packet summary and proof, and the plan remains
  narrowly tied to idea 184 rather than broad Phase E cleanup.
