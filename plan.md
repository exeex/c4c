# Phase F5 edge_publication_source_producers x86 Consumer Boundary Gate

Status: Active
Source Idea: ideas/open/281_phase_f5_edge_publication_source_producers_x86_consumer_boundary_gate.md

## Purpose

Name and prove, or explicitly block, the concrete x86 consumer boundary for
`PreparedFunctionLookups::edge_publication_source_producers`.

## Goal

Inspect x86 consumption of `edge_publication_source_producers`, identify whether
there is a real consumer boundary, and either prove one bounded x86 row or close
with the exact missing fixture or consumer support.

## Core Rule

Do not open implementation work until a real x86 consumer boundary is named.
If no boundary exists, record bounded non-applicability and the smallest
follow-up fixture-support idea instead of claiming parity progress.

## Read First

- `ideas/open/281_phase_f5_edge_publication_source_producers_x86_consumer_boundary_gate.md`
- Existing x86 prepared edge-publication tests and fixtures.
- Existing Route 5 edge-publication move support, especially copied-publication
  `LoadLocal` handling and non-`LoadLocal` fallback behavior.
- Any adjacent `edge_publication_source_producers` helper or lookup paths before
  deciding whether they are real backend consumers.

## Current Scope

- Inspect x86 consumption of
  `PreparedFunctionLookups::edge_publication_source_producers`.
- Name one concrete x86 consumer boundary if it exists.
- If supportable, prove one bounded x86 row through a real consumer while
  preserving compatible output and fallback behavior.
- If not supportable, record the exact missing consumer or fixture support and
  the smallest follow-up prerequisite.

## Non-Goals

- Do not fold in `memory_accesses`, `edge_publications` proof families,
  metadata, liveness, aggregate retirement, or draft 155 work.
- Do not claim non-`LoadLocal` fallback completion without direct real-consumer
  evidence.
- Do not weaken status/debug, route-debug, prepared-printer, helper/oracle,
  wrapper, exact-output, fallback, or baseline behavior.
- Do not treat helper-only or classification-only coverage as an x86 consumer
  boundary proof.

## Working Model

This is a boundary gate, not an implementation mandate. The first packet must
decide whether x86 has a concrete consumer of
`edge_publication_source_producers`. A supportable proof must drive that real
consumer. If the only available paths are helpers, adjacent lookups, partial
Route 5 copied-publication support, or fallback behavior without a public-field
consumer, the correct result is a precise blocker and prerequisite note.

## Execution Rules

- Keep changes narrow to inspection notes, targeted tests if a boundary exists,
  and `todo.md`.
- Prefer existing x86 fixture helpers and naming patterns over new abstractions.
- Do not add a consumer boundary just to satisfy this gate unless the supervisor
  activates a separate fixture-support idea first.
- Any proof must name the real x86 consumer, the public
  `edge_publication_source_producers` row, and the compatible output/fallback
  behavior being preserved.
- Run the supervisor-delegated proof command exactly when provided.
- Treat expectation downgrades, fallback relaxation, route-debug weakening,
  prepared-printer weakening, wrapper weakening, helper/oracle weakening,
  exact-output weakening, or baseline weakening as route drift.

## Step 1: Inspect x86 Source-Producer Consumers

Goal: determine whether x86 has a real consumer boundary for
`PreparedFunctionLookups::edge_publication_source_producers`.

Actions:

- Inspect x86 backend code, prepared edge-publication fixtures, and adjacent
  Route 5 edge-publication support.
- Separate real backend consumers from helper-only lookups, classification
  helpers, copied-publication internals, and adjacent `edge_publications`
  consumers.
- Record any `LoadLocal`-only support and any non-`LoadLocal` fallback blocker.
- Decide whether Step 2 should add a bounded proof or record
  non-applicability.

Completion check:

- `todo.md` names the x86 surface examined, states whether it is a real
  `edge_publication_source_producers` consumer boundary, and identifies the
  exact Step 2 route.

## Step 2: Prove Or Block The Boundary

Goal: either add one bounded real-consumer proof or record precise
non-applicability.

Actions:

- If Step 1 found a real boundary, construct one public
  `edge_publication_source_producers` row and drive the real x86 consumer.
- Assert compatible output and fallback behavior using the existing exact-output
  convention for the selected fixture.
- If Step 1 found no real boundary, record the absent consumer boundary and the
  smallest follow-up fixture-support or consumer-support prerequisite.
- Do not claim x86 parity when the proof is helper-only or when non-`LoadLocal`
  fallback remains unproven.

Completion check:

- The selected route is recorded in `todo.md`: either a focused real-consumer
  proof exists, or bounded non-applicability is documented with the exact
  missing support.

## Step 3: Preserve Compatible Behavior

Goal: ensure the gate does not weaken existing x86 compatible output or
fallback behavior.

Actions:

- For a supportable proof, keep or add assertions for the compatible row and
  fallback behavior adjacent to the selected consumer.
- For a blocked gate, name the existing x86 behavior that remains preserved
  without claiming new public-field parity.
- Confirm helper/oracle, prepared-printer, wrapper, route-debug, fallback,
  exact-output, and baseline contracts remain intact.

Completion check:

- `todo.md` records the compatible behavior preserved and explicitly states
  that no unsupported fallback completion was claimed.

## Step 4: Focused Acceptance Proof

Goal: produce the focused proof appropriate to the selected Step 2 route.

Actions:

- Run the supervisor-provided proof command exactly.
- If no command is provided and Step 2 added a test, use the narrow x86 backend
  test subset that covers the selected consumer.
- If Step 2 is todo-only non-applicability, run the focused x86 subset selected
  by the supervisor to prove nearby behavior still passes.
- Preserve `test_after.log` as the executor proof log when a test command is
  run.

Completion check:

- The focused proof is green and `todo.md` records the command, result, and
  confirmation that status/debug, route-debug, prepared-printer, helper/oracle,
  wrapper, exact-output, fallback, and baseline behavior were not weakened.

## Step 5: Closeout Notes

Goal: make the boundary-gate result durable without expanding this idea.

Actions:

- Record the examined x86 consumer surface and whether it is a real boundary.
- Record the proven row, or the exact missing fixture or consumer support.
- If blocked, name the smallest follow-up prerequisite for future work.
- Confirm this runbook did not perform broad x86 lowering, Route 5 rewrites,
  prepared API privatization, aggregate retirement, metadata/liveness, or draft
  155 work.

Completion check:

- `todo.md` contains enough closeout evidence for the plan owner to decide
  whether idea 281 is complete under its source acceptance criteria.
