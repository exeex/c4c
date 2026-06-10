# BIR Call-Use Source Annotation Schema Plan

Status: Active
Source Idea: ideas/open/164_bir_call_use_source_annotation_schema.md

## Purpose

Move Phase A Route 6 call-boundary semantic source facts into BIR-owned
`CallInst` instruction and value annotations while preserving prepared call
argument/result and direct-global dependency reads as migration oracles.

## Goal

Consumers can answer semantic call argument, call result, same-block producer,
direct-global dependency, and eligible memory/publication source questions from
typed BIR annotations without importing ABI, layout, scratch, or transport
policy.

## Core Rule

Keep call-use annotations semantic and target-neutral. Do not encode ABI
register names, stack slots, outgoing stack area sizing, variadic FPR counts,
clobber or preservation sets, byval lane layout, scratch resources,
helper/carrier protocols, destination homes, aggregate transport layout, or
ABI/layout-bound publication-routing source-selection reads in BIR
annotations.

## Read First

- `ideas/open/164_bir_call_use_source_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR `CallInst` annotations for argument source value/base value/name,
  same-block producer links, direct-global select-chain dependency, and
  semantic memory/publication source references where BIR has authority.
- BIR value annotations for call result provenance when consumers start from a
  produced result value.
- Function-local lookup/index helpers only where needed for cheap call-use
  answers over call/value annotations.
- Prepared call argument/result source materialization and direct-global
  dependency reads as temporary oracle checks until BIR-backed answers are
  proven equivalent.

## Non-Goals

- Do not migrate ABI register or stack placement, outgoing stack sizing,
  variadic FPR count, preservation or clobber sets, byval aggregate lanes,
  scratch requirements, destination homes, helper/carrier protocols, aggregate
  transport lane layout, or ABI/layout-bound publication-routing
  source-selection reads.
- Do not preserve whole `PreparedCallArgumentPlan`, call result plans,
  outgoing-stack plans, or aggregate transport lane records as the BIR
  annotation schema.
- Do not weaken call-boundary source contracts, result provenance cases,
  direct-global dependency cases, publication-source cases, or ABI-bound
  negative cases.
- Do not switch broad target/codegen or ABI assignment consumers before BIR and
  prepared answers are proven equivalent on accepted positive and negative
  cases.

## Working Model

- `CallInst` annotations own semantic call-use source facts at the instruction
  boundary.
- Value annotations own call result provenance that is meaningful when starting
  from the produced result value.
- Lookup helpers may be rebuilt from BIR call/value annotations, but they are
  indexes, not the semantic source of truth.
- ABI-bound answers must remain prepared/codegen-owned until a separate source
  idea justifies migration.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Use existing prepared call argument/result source materialization and
  direct-global dependency reads as oracle checks until a migration step
  explicitly switches a consumer to BIR-backed answers.
- Add negative coverage alongside positive call-use coverage, including result,
  direct-global, memory/publication-source, wrong call, missing source, and
  ABI-bound exclusion cases where applicable.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because shared call-use
  query helpers can affect both x86 and AArch64 backend callers.

## Steps

### Step 1: Inventory Call-Use Source Surfaces

Goal: identify the exact prepared call-use query surfaces, BIR payload targets,
and proof tests before adding schema code.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect prepared call argument/result source materialization,
  direct-global dependency, memory/publication source reference, and
  ABI-bound publication-routing query records and helper APIs.
- Record concrete `CallInst` annotation fields, value-annotation fields,
  optional function-local index shape, positive oracle cases, negative oracle
  cases, and the narrow proof command in `todo.md`.
- Identify the first low-risk test target for schema introduction.

Completion check:

- `todo.md` names the concrete schema fields, oracle helpers, expected
  result/direct-global/publication-source/ABI-bound negative cases, and
  delegated proof command for Step 2.

### Step 2: Add BIR Annotation Records

Goal: add typed BIR annotation records for Route 6 call-use facts without
switching production consumers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add `CallInst` annotation types for argument source value/base value/name,
  same-block producer links, direct-global select-chain dependency, and
  eligible semantic memory/publication source references.
- Add value annotation types for call result provenance.
- Add construction/query helpers that distinguish semantic missing-source or
  ABI-bound exclusions from absent annotation state.
- Add oracle test coverage that compares BIR annotation answers with existing
  prepared call-use answers for accepted positive and negative cases.

Completion check:

- Narrow backend tests prove the new BIR annotations are populated and match
  prepared call-use oracles without switching broad consumers.

### Step 3: Add Lookup/Index Helpers

Goal: provide stable BIR-id-based lookup helpers over the Step 2 annotations.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add function-local lookup/index helpers only where linear scans would make
  migration awkward for call argument/result and direct-global dependency
  queries.
- Keep the index rebuildable from BIR `CallInst` and value annotation payloads.
- Prove argument success, result provenance success, direct-global dependency
  success, eligible memory/publication source success, wrong call, missing
  source, and ABI-bound exclusion behavior.

Completion check:

- Tests demonstrate that lookup helpers are target-neutral indexes over BIR
  annotation payloads and fail closed for explicit negative cases.

### Step 4: Migrate A Low-Risk Query Consumer

Goal: switch the narrowest shared query helper to read BIR-backed Route 6 facts
while retaining prepared call-use oracle checks.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- backend tests identified in Step 1

Actions:

- Replace one low-risk prepared-owned semantic call-use answer with a
  BIR-backed annotation lookup.
- Keep prepared call-use results in tests as equivalence oracles during the
  migration.
- Avoid ABI assignment, target call lowering, aggregate transport, or
  AArch64-specific call sites unless the broader shared query layer is already
  proven.

Completion check:

- Narrow tests prove BIR-backed and prepared answers match for positive,
  result, direct-global, memory/publication-source, wrong-call,
  missing-source, ABI-bound exclusion, and no-match cases selected in Step 1.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the Route 6 schema is complete enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor,
  including AArch64-relevant coverage if shared query helpers changed.
- Confirm no expectation downgrades, ABI/layout policy leaks, whole prepared
  plan copies, target/codegen rewrites, or helper-only reshuffles remain.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
