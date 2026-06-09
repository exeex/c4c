# Plan: Comparison Prealloc Fact Owner

Status: Active
Source Idea: ideas/open/147_comparison_prealloc_fact_owner.md

## Purpose

Name and establish a narrow prealloc comparison owner for fused-compare and
materialized-condition producer facts that currently live under prepared
lookup ownership.

## Goal

Move comparison producer facts and lookup helper declarations into a concrete
comparison owner without changing branch-condition semantics or target-local
compare lowering.

## Core Rule

Name the comparison boundary before moving declarations. Do not treat
`control_flow.hpp` as a catch-all unless the comparison ownership boundary is
explicit and repo-local usage supports that choice.

## Read First

- `ideas/open/147_comparison_prealloc_fact_owner.md`
- Existing `src/backend/prealloc/` owners for source-producer materialization,
  control flow, and prepared lookup declarations.
- Current users of:
  - `PreparedFusedCompareOperandProducer`
  - `PreparedFusedCompareOperandProducerFacts`
  - `PreparedMaterializedConditionProducer`
  - fused-compare operand producer lookup helpers
  - materialized-condition producer lookup helpers

## Current Targets

- Add or reuse a concrete comparison owner such as
  `src/backend/prealloc/comparison.hpp` with a matching implementation file.
- Move only fused-compare operand producer and materialized-condition producer
  facts plus their lookup helper declarations.
- Keep the dependency on the same-block/source-producer materialization owner
  from idea 144.

## Non-Goals

- Do not move AArch64 compare instruction selection into prealloc.
- Do not move condition-code emission into prealloc.
- Do not change branch-condition semantics.
- Do not use line-count reduction as the reason for ownership.
- Do not add named-testcase shortcuts or target-local lowering policy to shared
  prealloc comparison facts.

## Working Model

Comparison prealloc ownership should sit above source-producer materialization
facts and below target-specific compare lowering. The moved APIs should expose
semantic producer facts that consumers can include narrowly, while preserving
the existing lowering behavior.

## Execution Rules

- Keep each code step buildable.
- Prefer narrow include changes that follow the new owner boundary.
- If shared branch or comparison semantics change, stop and escalate validation
  to full CTest.
- Proof for the accepted implementation must include:
  - `cmake --build --preset default`
  - `ctest --test-dir build -R '^backend_' --output-on-failure`

## Ordered Steps

### Step 1: Inspect Comparison Fact Ownership

Goal: Identify the exact declarations, definitions, and include consumers that
belong to the comparison fact owner.

Primary target: prepared lookup and prealloc comparison-related declarations.

Actions:

- Locate the current declarations and definitions for fused-compare operand
  producer facts and materialized-condition producer facts.
- Confirm which same-block/source-producer materialization owner APIs these
  facts depend on.
- List direct include consumers that should switch to the new owner once it
  exists.
- Decide whether a new `src/backend/prealloc/comparison.hpp` owner is clearer
  than an explicit comparison subsection under an existing control-flow owner.

Completion check:

- The chosen owner name and files are identified.
- No target-local compare lowering or branch emission code is included in the
  move set.

### Step 2: Establish The Comparison Owner

Goal: Create or extend the concrete prealloc comparison owner without moving
unrelated prepared lookup APIs.

Primary target: `src/backend/prealloc/` comparison/control-flow ownership.

Actions:

- Add the comparison owner header and implementation file, or extend the
  existing control-flow owner with an explicit comparison boundary if that is
  the repo-local pattern.
- Move the selected comparison fact type declarations into the owner header.
- Move corresponding implementation details into the owner implementation file.
- Keep dependencies on source-producer materialization facts explicit and
  narrow.

Completion check:

- The new owner compiles conceptually as an owner of comparison facts, not as a
  renamed prepared lookup bucket.
- The moved types still preserve their existing semantics.

### Step 3: Move Lookup Helpers And Consumers

Goal: Route fused-compare and materialized-condition lookup helpers through the
  comparison owner and update consumers.

Primary target: helper declarations, definitions, and include sites.

Actions:

- Move fused-compare operand producer lookup helper declarations to the
  comparison owner.
- Move materialized-condition producer lookup helper declarations to the
  comparison owner.
- Update implementation definitions to match the new ownership.
- Replace broad prepared lookup includes at consumers that only need the moved
  comparison facts or helpers.
- Leave consumers that still need `PreparedFunctionLookups` or
  `make_prepared_function_lookups` on the prepared lookup header.

Completion check:

- Consumers include the comparison owner where appropriate.
- No umbrella header hides the old prepared lookup facade under a different
  name.
- Behavior remains unchanged.

### Step 4: Validate And Record Proof

Goal: Prove the owner move is build-clean and backend tests still pass.

Primary target: build and backend test subset.

Actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- If shared branch/comparison semantics changed, run full CTest before
  acceptance.
- Record the exact proof commands and results in `todo.md`.

Completion check:

- Build and backend subset pass.
- Any broader validation required by semantic changes has passed.
- `todo.md` names the completed step and proof results.
