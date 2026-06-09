# Prepared Lookups Residual Owner Audit Runbook

Status: Active
Source Idea: ideas/open/141_prepared_lookups_residual_owner_audit.md

## Purpose

Audit the residual public and implementation surface in
`prepared_lookups.*` after ideas 136-140 moved the first domain-owned query
groups out of the broad prepared lookup facade.

## Goal

Classify every remaining public group in `prepared_lookups.hpp`, map its
definitions and consumers, and identify only concrete follow-up ideas where a
clearer owner boundary exists.

## Core Rule

This is analysis-only. Do not edit prealloc implementation, AArch64 codegen, or
tests while executing this runbook. Produce ownership evidence and focused
follow-up idea drafts instead of implementation changes.

## Read First

- `ideas/open/141_prepared_lookups_residual_owner_audit.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/select_chain_lookups.hpp`

## Current Targets

Residual prepared lookup groups to classify:

- move-bundle and return-chain lookup indexes
- value-home lookup indexes and `PreparedFunctionLookups`
- current-block entry publication lookup
- same-block scalar producer and integer-constant materialization queries
- call-argument source producer materialization queries
- fused-compare and materialized-condition producer facts
- same-block load-local stored-value source facts
- current-block join parallel-copy source facts and instruction-routing
  conveniences
- same-width stack-source and aggregate stack-source publication helpers

Consumer surfaces to inspect:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- any additional prealloc or backend consumers found by search

## Non-Goals

- Do not make direct code changes.
- Do not delete shared prepared facts or propose local BIR rescans,
  predecessor rescans, or name matching as replacements.
- Do not move AArch64 register spelling, scratch policy, hazard policy, or
  final instruction emission into shared prealloc code.
- Do not reopen select-chain, call-plan, addressing, or edge-publication work
  from ideas 137-140 unless a concrete residual dependency proves an unresolved
  owner boundary.
- Do not propose broad control-flow, out-of-SSA, call-planning, or backend
  redesign.

## Working Model

- `prepared_lookups.*` may remain the owner for central cached lookup data,
  index maps, and `PreparedFunctionLookups` wiring that many domains need.
- Narrow domain headers should own declarations when a residual group has a
  clear control-flow, publication, comparison, value-home, call, or same-block
  materialization owner.
- Reusable prepared facts and target-facing AArch64 routing conveniences should
  be distinguished explicitly, even when they temporarily remain in the same
  file.
- Consumer cleanup is valid only when it points consumers at a clearer owner
  API or narrower header without hiding the old facade behind a new name.

## Execution Rules

- Classify each residual group as exactly one primary category:
  `keep-core-aggregate`, `move-to-domain-owner`, `split-fact-from-routing`,
  `consumer-cleanup`, or `no-new-idea`.
- Record evidence for each classification: declaration location, definition
  location, construction path, direct consumers, and owner rationale.
- Treat line count alone as insufficient evidence for a move.
- Separate reusable facts from target-local routing and emission policy in the
  analysis.
- Draft follow-up ideas only for specific residual groups with named owner
  files and concrete acceptance criteria.
- No build proof is required for inspection-only steps. If execution
  accidentally discovers a required code change, stop and return the finding to
  the supervisor instead of editing implementation files.

## Step 1: Inventory residual prepared lookup public groups

Goal: enumerate every remaining public group in `prepared_lookups.hpp` and
assign each group an initial ownership question.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `ideas/open/141_prepared_lookups_residual_owner_audit.md`

Actions:

- Read the full residual public surface in `prepared_lookups.hpp`.
- Group declarations by semantic purpose rather than by line proximity alone.
- Include structs, lookup maps, query helpers, preparation helpers, and
  aggregate fields.
- Mark which groups overlap with the explicit targets from the source idea and
  which groups are additional residual public surface.
- Record unresolved ownership questions in `todo.md`.

Completion check:

- `todo.md` contains a residual public-group inventory that is complete enough
  for later definition and consumer mapping.
- No implementation files are edited.

## Step 2: Map definitions and construction wiring

Goal: connect each residual public group to its implementation and aggregate
construction path.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- relevant owner headers and implementation files from `Read First`

Actions:

- Map each public group to its definitions in `prepared_lookups.cpp`.
- Identify cached indexes, builder helpers, aggregate initialization, and
  dependencies on other prepared data.
- Note whether definitions are pure central lookup construction, domain
  semantic logic, or target-facing routing convenience.
- Record any owner header that already provides a narrower public home.

Completion check:

- `todo.md` contains a definition-location map and construction-wiring notes
  for the residual groups.
- No implementation files are edited.

## Step 3: Map consumers and include pressure

Goal: identify who still includes or calls residual prepared lookup APIs and
why.

Primary targets:

- AArch64 codegen files listed under `Current Targets`
- prealloc consumers found by search

Actions:

- Search for includes of `prepared_lookups.hpp` and direct calls to residual
  APIs.
- For each consumer, record which groups it uses and whether a narrower owner
  header or API already exists.
- Distinguish reusable prepared fact consumption from AArch64-local routing,
  emission, register, scratch, or hazard policy.
- Revisit idea 140's current-block join parallel-copy source facts and
  instruction routing explicitly.

Completion check:

- `todo.md` contains an AArch64 and prealloc consumer map with include/API
  pressure notes.
- Current-block join parallel-copy and stack-source publication helper
  ownership questions are explicitly covered.

## Step 4: Classify residual ownership

Goal: make the final ownership classification for every residual group.

Actions:

- Assign one primary category to each group:
  `keep-core-aggregate`, `move-to-domain-owner`,
  `split-fact-from-routing`, `consumer-cleanup`, or `no-new-idea`.
- Write explicit no-new-idea rationale for groups that should remain under
  `prepared_lookups.*`.
- Recommend the long-term role of `PreparedFunctionLookups`.
- Identify any group where evidence is insufficient and explain the exact
  ambiguity rather than guessing.

Completion check:

- `todo.md` contains the residual ownership table, no-new-idea notes, and
  `PreparedFunctionLookups` recommendation.
- No classification relies on line count alone or a named-case shortcut.

## Step 5: Draft closure payload and follow-up ideas

Goal: prepare the lifecycle close payload without performing implementation
work.

Actions:

- Consolidate the residual ownership table, definition-location map, consumer
  map, no-new-idea notes, and `PreparedFunctionLookups` recommendation.
- Draft concrete follow-up ideas only where the audit found a clearer owner
  move, facade split, or consumer cleanup.
- For each follow-up draft, name the residual group, proposed owner files,
  out-of-scope boundaries, proof route, and reviewer reject signals.
- If the residual surface is coherent, draft a no-follow-up closure note that
  explains why `prepared_lookups.*` is the correct central aggregate.

Completion check:

- `todo.md` contains a close-ready audit payload that satisfies the source
  idea's expected output.
- Any follow-up ideas are concrete enough for the supervisor to route to the
  plan owner for durable `ideas/open/` creation.
- No implementation files are edited.
