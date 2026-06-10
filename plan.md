# BIR Edge And Join-Source Annotation Schema Plan

Status: Active
Source Idea: ideas/open/163_bir_edge_join_source_annotation_schema.md

## Purpose

Move Phase A Route 5 edge publication and current-block join-source identity
facts into BIR-owned edge and block annotations while preserving prepared
edge/join queries as migration oracles.

## Goal

Consumers can answer predecessor/successor/destination keyed edge identity,
source identity, optional memory-source identity, no-source identity, and
current-block join-source questions from typed BIR annotations without
importing parallel-copy scheduling or move emission policy.

## Core Rule

Keep edge and join-source annotations semantic and target-neutral. Do not
encode move bundle order, parallel-copy step order, cycle temporary routing,
execution site/block placement, phase or carrier policy, coalescing and
redundancy flags, destination registers, storage-sharing checks, or prepared
move records in BIR annotations.

## Read First

- `ideas/open/163_bir_edge_join_source_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR edge annotations for predecessor block, successor block, destination
  value/name identity, source value/name/kind identity, source producer
  block/instruction, optional memory-source identity, and no-source markers.
- BIR block annotations for current-block join incoming source identity.
- Function-local lookup/index helpers for predecessor/successor/value keyed
  edge and join-source answers.
- Prepared edge publication, edge-copy source, and join-source queries as
  temporary oracle checks until BIR-backed answers are proven equivalent.

## Non-Goals

- Do not migrate move bundle order, parallel-copy step order, cycle temporary
  routing, execution site/block placement, phase or carrier resources,
  coalescing or redundancy policy, destination physical registers,
  storage-sharing checks, or prepared move records.
- Do not preserve whole `PreparedEdgePublication` records or prepared move
  records as the BIR annotation schema.
- Do not weaken edge-copy, no-source, memory-source, join-source, or negative
  cases.
- Do not switch broad target/codegen consumers before BIR and prepared answers
  are proven equivalent on accepted positive and negative cases.

## Working Model

- Edge annotations own semantic predecessor/successor/destination/source facts
  for CFG edge identity.
- Block annotations own current-block join incoming source identity that is
  meaningful at BIR level.
- Lookup helpers may be rebuilt from BIR annotations, but they are indexes, not
  the semantic source of truth.
- No-source and memory-source answers must be explicit semantic identities, not
  accidental absence of prepared move data.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Use existing prepared edge publication, edge-copy source, and join-source
  answers as oracle checks until a migration step explicitly switches a
  consumer to BIR-backed answers.
- Add negative coverage alongside positive edge/join coverage, including
  wrong predecessor, wrong successor, wrong destination, no-source,
  memory-source, and missing join-source cases where applicable.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because shared edge and
  join-source query helpers can affect both x86 and AArch64 backend callers.

## Steps

### Step 1: Inventory Edge And Join-Source Surfaces

Goal: identify the exact prepared edge/join query surfaces, BIR payload
targets, and proof tests before adding schema code.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect prepared edge publication, edge-copy source, and current-block
  join-source query records and helper APIs.
- Record concrete edge-annotation fields, block-annotation fields, optional
  function-local index shape, positive oracle cases, negative oracle cases, and
  the narrow proof command in `todo.md`.
- Identify the first low-risk test target for schema introduction.

Completion check:

- `todo.md` names the concrete schema fields, oracle helpers, expected
  no-source/memory-source/join-source negative cases, and delegated proof
  command for Step 2.

### Step 2: Add BIR Annotation Records

Goal: add typed BIR annotation records for Route 5 edge and join-source facts
without switching production consumers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add edge annotation types for predecessor/successor/destination/source
  identity, source producer block/instruction, optional memory-source identity,
  and no-source markers.
- Add block annotation types for current-block join incoming source identity.
- Add construction/query helpers that distinguish explicit no-source or
  missing-source facts from absent annotation state.
- Add oracle test coverage that compares BIR annotation answers with existing
  prepared edge/join query answers for accepted positive and negative cases.

Completion check:

- Narrow backend tests prove the new BIR annotations are populated and match
  prepared edge/join oracles without switching broad consumers.

### Step 3: Add Lookup/Index Helpers

Goal: provide stable BIR-id-based lookup helpers over the Step 2 annotations.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add function-local lookup/index helpers only where linear scans would make
  migration awkward for predecessor/successor/value keyed edge and join-source
  queries.
- Keep the index rebuildable from BIR edge/block annotation payloads.
- Prove edge success, join-source success, no-source, memory-source, wrong
  predecessor, wrong successor, wrong destination, and missing-source behavior.

Completion check:

- Tests demonstrate that lookup helpers are target-neutral indexes over BIR
  annotation payloads and fail closed for explicit negative cases.

### Step 4: Migrate A Low-Risk Query Consumer

Goal: switch the narrowest shared query helper to read BIR-backed Route 5 facts
while retaining prepared edge/join oracle checks.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- backend tests identified in Step 1

Actions:

- Replace one low-risk prepared-owned semantic edge or join-source answer with
  a BIR-backed annotation lookup.
- Keep prepared edge/join results in tests as equivalence oracles during the
  migration.
- Avoid target-specific parallel-copy scheduling, move emission, or AArch64
  call sites unless the broader shared query layer is already proven.

Completion check:

- Narrow tests prove BIR-backed and prepared answers match for positive,
  no-source, memory-source, wrong-edge, missing-source, and no-match cases
  selected in Step 1.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the Route 5 schema is complete enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor,
  including AArch64-relevant coverage if shared query helpers changed.
- Confirm no expectation downgrades, parallel-copy policy leaks, whole
  prepared record copies, target/codegen rewrites, or helper-only reshuffles
  remain.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
