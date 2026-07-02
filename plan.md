# BIR Route7 Comparison Body Extraction Runbook

Status: Active
Source Idea: ideas/open/527_bir_route7_comparison_body_extraction.md

## Purpose

Extract route7 comparison record-construction implementation bodies without
moving facade-backed public query helpers in the same slice.

## Goal

Move route7 comparison record-construction bodies into a focused implementation
owner while preserving public declarations, facade consumers, and comparison
semantics.

## Core Rule

This is a behavior-preserving body extraction only. Do not change comparison
operand producer semantics, materialized-condition decisions, fused-compare
behavior, or route-index facade behavior to make the move compile or pass.

## Read First

- `ideas/open/527_bir_route7_comparison_body_extraction.md`
- Current route7 declarations in `bir.hpp`
- Current route7 implementation bodies in `bir.cpp`
- Facade-backed materialized-condition and fused-compare public query helpers
- `.codex/skills/c4c-clang-tools/` before mapping symbols or moving code

## Current Targets

- Route7 comparison record-construction bodies
- New `bir_route7_comparison.cpp` only if a focused owner file is needed
- Private helper declarations only when required to preserve existing behavior
- Existing public declarations in `bir.hpp`

## Non-Goals

- Do not move route-index facade bodies.
- Do not move facade-backed materialized-condition public query helpers.
- Do not move facade-backed fused-compare public query helpers.
- Do not change comparison operand producer semantics.
- Do not add idea 422 producer behavior.
- Do not split route7 public declarations out of `bir.hpp`.
- Do not mix header splitting into this body-only movement.
- Do not rewrite tests to weaken materialized-condition or comparison
  contracts.

## Working Model

Route7 owns comparison record construction. Materialized-condition and
fused-compare public queries are cross-route or facade consumers, so they should
remain in their existing translation unit for this slice. The extraction should
move only the cohesive record-construction bodies behind the same public
surface, preserving record identity, operand selection, optionality, and lookup
behavior.

## Execution Rules

- Use AST-backed symbol queries to map route7 symbols, direct callers, direct
  callees, facade users, and comparison type references before editing.
- Move bodies in small compileable steps and avoid opportunistic cleanup.
- Keep public route7 declarations in `bir.hpp`.
- Add private helper declarations only for moved existing helpers that need a
  shared internal surface.
- Keep materialized-condition and fused-compare public query helpers out of the
  new route7 owner file unless a later source idea explicitly owns that move.
- Treat comparison operand producer changes as route drift.
- Run fresh build proof after code changes.
- Run focused comparison condition indexing and materialized-condition consumer
  proof selected by the supervisor.
- Include link-time backend proof because public query helpers remain outside
  the new route7 owner translation unit.

## Ordered Steps

### Step 1: Map Route7 Comparison Boundaries

Goal: identify the exact route7 record-construction bodies, dependencies, and
facade consumers before moving any body.

Primary target: current route7 symbols in `bir.hpp` and `bir.cpp`.

Actions:

- Use `c4c-clang-tools` to list route7 declarations and definitions.
- Query direct callers and callees for route7 comparison construction helpers.
- Map materialized-condition and fused-compare public query helpers that must
  stay out of this body move.
- Map comparison type references and route-index facade users.
- Record any helper declarations that must remain private after the body move.

Completion check:

- The executor can name the route7 bodies to move, the facade-backed helpers
  that must not move, and the focused proof needed for comparison and
  materialized-condition consumers.

### Step 2: Create Route7 Comparison Owner

Goal: introduce the focused implementation owner without changing behavior.

Primary target: `bir_route7_comparison.cpp`.

Actions:

- Add the new route7 owner translation unit if no suitable focused file exists.
- Move existing route7 comparison record-construction bodies into it.
- Keep public declarations in `bir.hpp`.
- Add only required private declarations or includes for the moved bodies.
- Preserve existing namespaces, signatures, record identity, operand selection,
  optionality, and lookup behavior.

Completion check:

- The project compiles far enough to prove the moved bodies are linked once,
  no public route7 declaration moved out of `bir.hpp`, and facade-backed public
  query helpers stayed outside the new owner.

### Step 3: Repair Includes And Internal Linkage

Goal: resolve compile and linkage fallout from the body move without expanding
scope.

Primary target: includes, private helper declarations, and build registration
for the route7 owner file.

Actions:

- Add the route7 owner file to the repo-native build registration.
- Tighten includes only where the moved bodies require them.
- Keep helper visibility no broader than necessary.
- Do not duplicate facade, materialized-condition, or fused-compare logic to
  avoid dependency cleanup.

Completion check:

- Fresh build proof passes for the delegated build target.

### Step 4: Focused Route7 Proof

Goal: prove that comparison record construction and its consumers stayed
stable.

Primary target: supervisor-selected focused backend tests.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include comparison condition indexing proof.
- Include materialized-condition consumer proof.
- Include link-time backend proof that exercises the separated public query
  helpers.
- Investigate failures as behavior-preservation issues, not expectation-edit
  opportunities.

Completion check:

- Focused comparison condition indexing, materialized-condition consumer, and
  link-time backend proof passes without weaker expectations.

### Step 5: Acceptance Checkpoint

Goal: hand the completed body extraction back with enough proof for supervisor
review.

Primary target: final diff, `todo.md`, and canonical proof log.

Actions:

- Confirm the diff contains route7 record-construction body movement and
  minimal build/include support only.
- Confirm route7 public declarations remain in `bir.hpp`.
- Confirm facade-backed materialized-condition and fused-compare public query
  helpers did not move.
- Confirm comparison operand producer semantics did not change.
- Leave proof results in the canonical executor log requested by the
  supervisor.

Completion check:

- The supervisor can review the slice against the source idea and decide
  whether broader validation or commit is ready.
