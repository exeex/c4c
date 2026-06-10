# BIR Publication Availability Annotation Schema Plan

Status: Active
Source Idea: ideas/open/162_bir_publication_availability_annotation_schema.md

## Purpose

Move Phase A Route 4 block-entry and current-block publication availability
facts into BIR-owned block/value annotations while preserving prepared
publication reads as migration oracles.

## Goal

Consumers can answer block-entry availability, current-block availability,
produced/consumed value identity, source producer identity, and producer kind
questions from typed BIR annotations without importing publication emission or
storage choices.

## Core Rule

Keep availability semantic and target-neutral. Do not encode publication hooks,
destination homes, storage encodings, stack-source extension policy, register
views, immediate payload spelling, emitted storage availability, or scalar
publication emission order in BIR annotations.

## Read First

- `ideas/open/162_bir_publication_availability_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR block annotations for value availability at block entry and current
  program points.
- BIR value annotations for produced value identity, consumed value identity,
  source producer identity, and source producer kind references.
- Function-local lookup/index helpers for cheap availability answers over
  block/value annotations.
- Prepared publication read paths as temporary oracle checks until BIR-backed
  answers are proven equivalent.

## Non-Goals

- Do not migrate publication hooks, destination homes, storage encodings,
  stack-source extension policy, register-view conversion, immediate
  publication payload spelling, emitted storage availability, or scalar
  publication emission policy.
- Do not preserve whole prepared publication plan records or
  `PreparedCurrentBlockPublicationConsumption` as the BIR annotation schema.
- Do not weaken unavailable, block-entry, alternate-source, or negative
  publication cases.
- Do not switch broad target consumers before BIR and prepared answers are
  proven equivalent on accepted positive and negative cases.

## Working Model

- Block annotations own semantic availability at block entry and current block
  positions.
- Value annotations own produced/consumed value identity and source producer
  references that are meaningful at BIR level.
- Lookup helpers may be rebuilt from BIR annotations, but they are indexes, not
  the semantic source of truth.
- Unavailable answers must be explicit enough that consumers do not infer
  availability from storage or emission policy.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Use existing prepared publication reads as oracle checks until a migration
  step explicitly switches a consumer to BIR-backed answers.
- Add negative coverage alongside positive availability coverage, including
  block-entry unavailable, current-block unavailable, alternate producer, and
  non-matching source cases where applicable.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because shared
  publication query helpers can affect both x86 and AArch64 backend callers.

## Steps

### Step 1: Inventory Publication Availability Surfaces

Goal: identify the exact prepared publication read surfaces, BIR payload
targets, and proof tests before adding schema code.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect prepared current-block and block-entry publication availability
  records, value identity payloads, source producer identity payloads, and
  helper APIs.
- Record concrete block-annotation fields, value-annotation fields, optional
  function-local index shape, positive oracle cases, negative oracle cases, and
  the narrow proof command in `todo.md`.
- Identify the first low-risk test target for schema introduction.

Completion check:

- `todo.md` names the concrete schema fields, oracle helpers, expected
  unavailable/alternate-source cases, and delegated proof command for Step 2.

### Step 2: Add BIR Annotation Records

Goal: add typed BIR annotation records for Route 4 availability facts without
switching production consumers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add block annotation types for block-entry and current-block value
  availability.
- Add value annotation types for produced value identity, consumed value
  identity, source producer identity, and source producer kind references.
- Add construction/query helpers that distinguish explicit unavailable facts
  from absent annotation state.
- Add oracle test coverage that compares BIR annotation answers with existing
  prepared publication reads for accepted positive and negative cases.

Completion check:

- Narrow backend tests prove the new BIR annotations are populated and match
  prepared publication oracles without switching broad consumers.

### Step 3: Add Lookup/Index Helpers

Goal: provide stable BIR-id-based lookup helpers over the Step 2 annotations.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add function-local lookup/index helpers only where linear scans would make
  migration awkward for block-entry and current-block availability queries.
- Keep the index rebuildable from BIR block/value annotation payloads.
- Prove block-entry success, current-block success, explicit unavailable,
  alternate source producer, and non-matching value behavior.

Completion check:

- Tests demonstrate that lookup helpers are target-neutral indexes over BIR
  annotation payloads and fail closed for explicit negative cases.

### Step 4: Migrate A Low-Risk Query Consumer

Goal: switch the narrowest shared query helper to read BIR-backed Route 4 facts
while retaining prepared publication oracle checks.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- backend tests identified in Step 1

Actions:

- Replace one low-risk prepared-owned semantic availability answer with a
  BIR-backed annotation lookup.
- Keep prepared publication results in tests as equivalence oracles during the
  migration.
- Avoid target-specific publication emission, storage, or AArch64 call sites
  unless the broader shared query layer is already proven.

Completion check:

- Narrow tests prove BIR-backed and prepared answers match for positive,
  unavailable, block-entry, current-block, alternate-source, and no-match cases
  selected in Step 1.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the Route 4 schema is complete enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor,
  including AArch64-relevant coverage if shared query helpers changed.
- Confirm no expectation downgrades, publication-policy leaks, whole prepared
  publication record copies, or helper-only reshuffles remain.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
