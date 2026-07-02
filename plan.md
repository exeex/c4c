# BIR Memory Provenance Header Readiness Runbook

Status: Active
Source Idea: ideas/open/531_bir_memory_provenance_header_readiness.md

## Purpose

Prepare a behavior-preserving decision on whether BIR memory provenance and
storage-authority declarations can move behind a narrower public support
header after the route-header cleanup work.

## Goal

Audit memory provenance, storage authority, static GEP, dynamic-array, route3,
and LIR-to-BIR memory consumers, then move declarations only if the evidence
shows a narrower public header reduces coupling without semantic churn.

## Core Rule

This is declaration-surface cleanup only. Do not change pointer provenance,
static GEP authority, object extent, byte-range, dynamic-array, storage
authority semantics, record layout, enum values, verdicts, or implementation
behavior.

## Read First

- `ideas/open/531_bir_memory_provenance_header_readiness.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_index_prereqs.hpp`
- route3, LIR-to-BIR memory, object emission, and pointer-value provenance
  consumers found through clang-backed symbol and type-reference queries

## Current Targets

- memory provenance declarations and support types in the public BIR model
- storage authority and byte-range/object-extent declaration clusters
- static GEP and dynamic-array declaration dependencies
- route3 memory access consumers
- LIR-to-BIR memory lowering consumers
- object emission and pointer-value provenance consumers

## Non-Goals

- Do not move memory provenance declarations into private `lir_to_bir/memory/`
  headers.
- Do not edit idea 422 producer behavior.
- Do not split or move core `MemoryAddress` usage unless a separate core-header
  idea owns that work.
- Do not repair memory lowering behavior, admission policy, diagnostics, or
  tests in this cleanup route.
- Do not change capability expectations, unsupported markers, allowlists,
  runtime checks, or pass/fail accounting.

## Working Model

`bir.hpp` remains the compatibility aggregator unless the evidence proves a
standalone public memory provenance support header is safe. A new header is
acceptable only if it is self-contained enough for its selected public
consumers and does not create include cycles with route, core model, or
lowering headers.

## Execution Rules

- Use `.codex/skills/c4c-clang-tools/` before raw long-file reading for
  symbol lists, function signatures, caller/callee checks, and type-reference
  queries.
- Prefer mapping-only packets before code movement.
- Keep each code-changing packet behavior-preserving and paired with build
  proof plus focused backend proof selected by the supervisor.
- If a candidate boundary depends on `Function`, `MemoryAddress`, route-index,
  or local-array declarations in a way that prevents standalone use, record the
  blocker in `todo.md` instead of forcing an include cycle.
- If movement proves unsafe under this idea, park the route with the blocker
  named explicitly rather than widening scope.

## Step 1: Map Memory Provenance Boundaries

Goal: identify the declaration clusters, complete-type requirements, and
consumer families before selecting any header boundary.

Primary targets:

- memory provenance and storage authority declarations in `src/backend/bir/`
- route3 memory access consumers
- LIR-to-BIR memory lowering consumers
- object emission and pointer-value provenance consumers

Actions:

- run clang-backed symbol and type-reference queries for provenance,
  authority, static GEP, dynamic-array, route3, and memory lowering types
- record direct and transitive include users with `rg`
- probe whether a candidate public memory provenance header would need full
  `Function`, `MemoryAddress`, route-index, local-array, or producer types
- identify the first safe Step 2 packet, or recommend parking if every useful
  boundary is blocked by core model dependencies

Completion check:

- `todo.md` names the candidate declaration boundary, required prerequisites,
  unsafe include replacements, and exact proof command for the next
  code-changing packet, or states that no safe declaration movement exists
  under idea 531.

## Step 2: Split Only A Proven Safe Public Support Boundary

Goal: move the smallest memory provenance support declaration cluster that
reduces coupling without changing behavior.

Actions:

- add or update a focused public header only for declarations proven safe by
  Step 1
- keep `bir.hpp` as the compatibility aggregator
- avoid direct include replacement unless the new header is standalone for that
  user
- preserve public names, enum values, storage layout, optionality, lookup
  behavior, and verdicts

Completion check:

- build proof passes
- focused route3 memory access, LIR-to-BIR memory lowering, object emission,
  and pointer-value provenance proof passes as selected by the supervisor
- `todo.md` records whether the new header is aggregator-only or suitable for
  direct include use

## Step 3: Validate Direct Include Opportunities Or Park

Goal: determine whether any public consumers can include the new support
header directly without depending on `bir.hpp`.

Actions:

- test direct no-file compile probes for the new header and selected consumers
- replace includes only when the replacement is independently buildable and
  reduces coupling
- leave consumers on `bir.hpp` when prerequisites remain outside idea 531
- record any remaining prerequisite as a separate future idea instead of
  expanding this route

Completion check:

- direct include replacements are either proven and validated, or explicitly
  rejected with the blocking complete-type/include-cycle reason in `todo.md`

## Step 4: Close Or Park The Memory Provenance Header Route

Goal: decide whether idea 531 is complete after the mapping and any safe header
movement.

Actions:

- summarize the final declaration-surface state in `todo.md`
- confirm no provenance authority verdicts, record layouts, storage semantics,
  or lowering behavior changed
- identify whether `ideas/open/532_bir_local_array_semantic_gep_header_readiness.md`
  is unblocked next

Completion check:

- supervisor-selected close proof passes
- lifecycle owner can either close idea 531 or park it with a precise blocker
  without losing execution knowledge
