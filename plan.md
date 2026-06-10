# BIR Select-Chain Global Dependency Annotation Schema Plan

Status: Active
Source Idea: ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md

## Purpose

Move Phase A Route 2 select-chain and direct-global dependency facts into
BIR-owned value and instruction annotations while preserving prepared-query
answers as migration oracles.

## Goal

Consumers can read accepted select-chain source identity, root producer,
direct-global dependency, and scalar materialization eligibility facts from
typed BIR annotations with explicit negative behavior.

## Core Rule

Keep the schema target-neutral. Do not encode AArch64 instruction spelling,
target move choices, materialization cost, hazard policy, register availability,
publication routing, or call ABI behavior into these annotations.

## Read First

- `ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR value annotations for select-root identity, root-is-select, scalar
  eligibility, and direct-global dependency summary.
- BIR instruction annotations for root producer instruction/index and direct
  `LoadGlobalInst` dependency.
- Function-local lookup/index helpers only where they make stable BIR-id-based
  lookup cheap.
- Prepared select-chain query paths as temporary oracle checks.

## Non-Goals

- Do not migrate target materialization cost, hazard decisions, register
  availability, publication routing policy, call ABI behavior, or final
  AArch64 move/branch choices.
- Do not preserve whole prepared records as the BIR annotation schema.
- Do not weaken or mark existing select-chain or direct-global cases
  unsupported.
- Do not switch broad target consumers before BIR and prepared answers are
  proven equivalent on accepted positive and negative cases.

## Working Model

- Value annotations own durable value/root/dependency summaries.
- Instruction annotations own root producer and direct load-global producer
  facts.
- Lookup helpers may be rebuilt from BIR annotations, but they are indexes, not
  the semantic source of truth.
- Missing annotations must represent explicit negative answers, not accidental
  cache misses.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Use existing prepared query answers as oracle checks until a migration step
  explicitly switches a consumer to BIR-backed answers.
- Add negative coverage alongside positive select-chain and direct-global
  coverage.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because the query layer
  can affect shared x86 and AArch64 backend callers.

## Steps

### Step 1: Inventory Select-Chain Surfaces

Goal: identify the exact prepared query surfaces, BIR payload targets, and
proof tests before adding schema code.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect prepared select-chain, direct-global dependency, and scalar
  materialization query records and helper APIs.
- Record concrete value-annotation fields, instruction-annotation fields,
  optional function-local index shape, positive oracle cases, negative oracle
  cases, and the narrow proof command in `todo.md`.
- Identify the first low-risk test target for schema introduction.

Completion check:

- `todo.md` names the concrete schema fields, oracle helpers, expected negative
  cases, and delegated proof command for Step 2.

### Step 2: Add BIR Annotation Records

Goal: add typed BIR annotation records for Route 2 facts without switching
production consumers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add value and instruction annotation types for select-root identity,
  root-is-select, scalar eligibility, root producer instruction/index, and
  direct `LoadGlobalInst` dependency.
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
  migration awkward.
- Keep the index rebuildable from BIR annotation payloads.
- Prove direct-global success, no-dependency, select-root, and non-select-root
  behavior.

Completion check:

- Tests demonstrate that lookup helpers are target-neutral indexes over BIR
  annotation payloads and fail closed for explicit negative cases.

### Step 4: Migrate A Low-Risk Query Consumer

Goal: switch the narrowest shared query helper to read BIR-backed Route 2 facts
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
- Avoid target-specific call sites unless the broader shared query layer is
  already proven.

Completion check:

- Narrow tests prove BIR-backed and prepared answers match for positive,
  no-dependency, and non-select-root cases.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the Route 2 schema is complete enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor, including
  AArch64-relevant coverage if shared query helpers changed.
- Confirm no expectation downgrades, target-policy leaks, or helper-only
  reshuffles remain.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
