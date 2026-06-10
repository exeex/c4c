# BIR Memory Access Identity Annotation Schema Plan

Status: Active
Source Idea: ideas/open/161_bir_memory_access_identity_annotation_schema.md

## Purpose

Move Phase A Route 3 memory/access identity facts into BIR-owned instruction
and value annotations while preserving prepared memory/access queries as
migration oracles.

## Goal

Consumers can answer direct memory access, same-block global-load identity, and
same-block load-local source identity questions from typed BIR annotations
without importing target layout or addressing policy.

## Core Rule

Keep the schema semantic and target-neutral. Do not encode frame slot ids, byte
offsets, size/align layout, relocation spelling, TLS register details,
base-plus-offset legality, addressing-mode choice, or AArch64 memory operand
formation into these annotations.

## Read First

- `ideas/open/161_bir_memory_access_identity_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR instruction annotations for load/store/address-node identity,
  address-space, volatile flag, semantic base kind, and source
  local/global/pointer/string identity where BIR can represent it.
- BIR value annotations for load result values, store source values, and
  same-block load-local stored-value source links.
- Function-local lookup/index helpers for cheap same-block global-load and
  load-local queries over annotation payloads.
- Prepared memory/access query paths as temporary oracle checks.

## Non-Goals

- Do not migrate `PreparedAddress` wholesale, frame slot ids, concrete byte
  offsets, size/align layout, base-plus-offset legality, relocation spelling,
  TLS register details, GOT/direct/page-low policy, target addressing-mode
  choice, or AArch64 memory operand legality.
- Do not preserve whole prepared memory/access records as the BIR annotation
  schema.
- Do not weaken or mark existing memory/access cases unsupported.
- Do not switch broad target consumers before BIR and prepared answers are
  proven equivalent on accepted positive and negative cases.

## Working Model

- Instruction annotations own durable access identity, semantic base, and
  source identity facts.
- Value annotations own result/source value links that are meaningful at BIR
  level.
- Lookup helpers may be rebuilt from BIR annotations, but they are indexes, not
  the semantic source of truth.
- Missing annotations must represent explicit negative answers, not accidental
  cache misses.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Use existing prepared query answers as oracle checks until a migration step
  explicitly switches a consumer to BIR-backed answers.
- Add negative coverage alongside positive memory/access coverage, including
  volatile, address-space, local-source, and no-source cases where applicable.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because shared memory
  query helpers can affect both x86 and AArch64 backend callers.

## Steps

### Step 1: Inventory Memory Access Surfaces

Goal: identify the exact prepared query surfaces, BIR payload targets, and
proof tests before adding schema code.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect prepared direct memory access, same-block global-load identity, and
  same-block load-local source query records and helper APIs.
- Record concrete instruction-annotation fields, value-annotation fields,
  optional function-local index shape, positive oracle cases, negative oracle
  cases, and the narrow proof command in `todo.md`.
- Identify the first low-risk test target for schema introduction.

Completion check:

- `todo.md` names the concrete schema fields, oracle helpers, expected negative
  cases, and delegated proof command for Step 2.

### Step 2: Add BIR Annotation Records

Goal: add typed BIR annotation records for Route 3 facts without switching
production consumers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add instruction annotation types for load/store/address identity,
  address-space, volatile flag, semantic base kind, and source
  local/global/pointer/string identity where BIR can represent it.
- Add value annotation types for load result values, store source values, and
  same-block load-local stored-value source links.
- Add construction/query helpers that distinguish present-negative facts from
  absent annotation state.
- Add oracle test coverage that compares BIR annotation answers with the
  existing prepared query answers for accepted positive and negative cases.

Completion check:

- Narrow backend tests prove the new BIR annotations are populated and match
  prepared-query oracles without switching broad consumers.

### Step 3: Add Lookup/Index Helpers

Goal: provide stable BIR-id-based lookup helpers over the Step 2 annotations.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add function-local lookup/index helpers only where linear scans would make
  migration awkward for same-block global-load and load-local source queries.
- Keep the index rebuildable from BIR annotation payloads.
- Prove global-load success, load-local source success, volatile or
  address-space separation, no-source, and non-matching access behavior.

Completion check:

- Tests demonstrate that lookup helpers are target-neutral indexes over BIR
  annotation payloads and fail closed for explicit negative cases.

### Step 4: Migrate A Low-Risk Query Consumer

Goal: switch the narrowest shared query helper to read BIR-backed Route 3 facts
while retaining prepared-query oracle checks.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- backend tests identified in Step 1

Actions:

- Replace one low-risk prepared-owned semantic answer with BIR-backed
  annotation lookup.
- Keep prepared query results in tests as equivalence oracles during the
  migration.
- Avoid target-specific addressing or AArch64 call sites unless the broader
  shared query layer is already proven.

Completion check:

- Narrow tests prove BIR-backed and prepared answers match for positive,
  volatile, address-space, local-source, and no-source cases selected in Step
  1.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the Route 3 schema is complete enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor, including
  AArch64-relevant coverage if shared query helpers changed.
- Confirm no expectation downgrades, target-policy leaks, whole-prepared-record
  schema copies, or helper-only reshuffles remain.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
