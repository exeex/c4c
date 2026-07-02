# BIR Local-Array And Semantic-GEP Header Readiness Runbook

Status: Active
Source Idea: ideas/open/532_bir_local_array_semantic_gep_header_readiness.md

## Purpose

Turn the local-array proof and semantic-GEP declaration-surface idea into a
small, behavior-preserving header-readiness runbook.

## Goal

Decide whether local-array proof records and semantic-GEP records can move
behind narrower BIR analysis headers now that memory provenance ownership is
settled, and make only the safe declaration-surface split if include/build
evidence supports it.

## Core Rule

This is declaration-surface cleanup only. Do not change record layout, vector
storage, optionality, lookup behavior, lowering behavior, authority policy, or
capability tests.

## Read First

- `ideas/open/532_bir_local_array_semantic_gep_header_readiness.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_memory_provenance.hpp`
- local-array proof, semantic-GEP, scalar local-load, and static-GEP authority
  tests or call sites discovered by symbol queries
- `.codex/skills/c4c-clang-tools/SKILL.md` before using C++ structure queries

## Current Targets

- Local-array proof record declarations and storage surfaces.
- Semantic-GEP record declarations and lookup surfaces.
- Scalar local-load consumers.
- Static-GEP authority consumers.
- Include relationships among `bir.hpp`, `bir_memory_provenance.hpp`, and any
  proposed analysis header.

## Non-Goals

- Do not move implementation behavior in this plan.
- Do not change `Function` storage semantics.
- Do not change memory provenance authority policy.
- Do not fold this work into LIR-to-BIR producer capability.
- Do not edit expectations, unsupported markers, allowlists, or behavior tests
  to justify a split.
- Do not replace direct includes when consumers still require complete core BIR
  model, route, or lowering declarations from existing aggregate headers.

## Working Model

The prior memory-provenance split established that some BIR analysis
declarations can move to focused headers while `bir.hpp` remains the
compatibility aggregator. This plan must re-check local-array and semantic-GEP
dependencies independently; it may finish by parking direct include
replacement if the current consumers still need broader declarations.

## Execution Rules

- Use AST-backed `c4c-clang-tool` or `c4c-clang-tool-ccdb` type-reference,
  declaration, caller/callee, and include evidence before selecting a boundary.
- Prefer a new focused header only when it preserves the aggregator path through
  `bir.hpp` and does not create cycles with memory provenance or route headers.
- Keep code changes narrow: declarations may move; behavior must not.
- For code-changing steps, run a fresh build proof and focused backend tests.
- Escalate to broad backend subset proof if `Function` storage declarations or
  cross-cutting BIR model declarations are touched.

## Steps

### Step 1: Audit Declaration Clusters And Consumers

Goal: Map the local-array proof and semantic-GEP declaration surfaces before
moving anything.

Primary target: `src/backend/bir/bir.hpp`

Actions:

- Load `c4c-clang-tools` and query symbols/types for local-array proof records,
  semantic-GEP records, scalar local-load consumers, and static-GEP authority
  consumers.
- Identify which declarations require complete `Value`, `MemoryAddress`,
  `Inst`, `Block`, `Function`, route, lowering, or memory-provenance types.
- Record the candidate declaration clusters and any include-cycle risks in
  `todo.md`.

Completion check:

- `todo.md` names the candidate clusters, required complete types, unsafe direct
  include replacements, and the first safe Step 2 boundary.

### Step 2: Extract Only A Safe Analysis Header Boundary

Goal: Create a focused declaration header only if Step 1 proves a safe,
behavior-preserving boundary.

Primary target: a new or existing BIR analysis header under
`src/backend/bir/`, if justified by Step 1.

Actions:

- Move only declarations that Step 1 proves can live in a focused header.
- Include the focused header from `bir.hpp` at the original prerequisite-safe
  boundary so existing consumers keep compiling.
- Preserve declaration names, layouts, field types, storage containers,
  optionality, and lookup signatures.
- If no safe header boundary exists, do not force a split; record the blocker in
  `todo.md` and stop for supervisor review.

Completion check:

- Either a focused header exists and `bir.hpp` remains the compatibility
  aggregator, or `todo.md` records why the split is parked without source edits.

### Step 3: Probe Consumer Include Narrowing

Goal: Decide whether any consumers can include the focused header directly.

Primary target: consumers discovered in Step 1.

Actions:

- Use temporary syntax-only include probes before editing consumer includes.
- Replace direct `bir.hpp` includes only where the consumer compiles with the
  focused header and no longer needs complete core BIR model, route, or lowering
  declarations.
- Leave consumers on `bir.hpp` when complete declarations are still required.
- Record parked consumers and their required declarations in `todo.md`.

Completion check:

- Any include replacements are backed by syntax/build evidence, and unsafe
  replacements are explicitly parked in `todo.md`.

### Step 4: Validate Behavior-Preserving Result

Goal: Prove the declaration-surface cleanup did not alter BIR behavior or
backend coverage.

Actions:

- Run `cmake --build --preset default`.
- Run focused backend tests covering local-array proof, semantic GEP, scalar
  local-load, and static-GEP authority.
- If `Function` storage declarations or cross-cutting BIR model declarations
  were touched, run the broad backend subset:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Write the final proof command and result to `test_after.log` when validation
  is delegated as an execution packet.

Completion check:

- Required proof is green, `todo.md` records the proof, and the diff contains
  only behavior-preserving declaration/header changes plus lifecycle state.

## Completion Criteria

- Build proof passes.
- Focused local-array proof, semantic GEP, scalar local-load, and static-GEP
  authority proof passes.
- Broad backend subset is considered and run when `Function` storage or
  cross-cutting BIR model declarations are touched.
- No include cycles appear between core model, memory provenance, and route
  headers.
- No lowering behavior or capability tests are edited to justify the split.
