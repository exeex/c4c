# BIR Producer Identity Annotation Schema Runbook

Status: Active
Source Idea: ideas/open/159_bir_producer_identity_annotation_schema.md

## Purpose

Prototype a target-neutral BIR annotation schema for Route 1 producer/source
identity facts without importing prepared publication, storage, register, or
final-emission policy.

## Goal

Make BIR the owner of producer/source identity for same-block producer,
constant, source value/name/type, producer instruction/index, and
materialization-availability queries, with prepared queries retained as
migration oracles.

## Core Rule

The durable schema may name BIR values, instructions, types, constants,
producer kinds, and lookup keys. It must not contain `PreparedValueHome`,
physical registers, storage encodings, frame slots, emitted-register state,
spill/reload behavior, operand views, or final instruction records.

## Read First

- `ideas/open/159_bir_producer_identity_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets And Scope

- Define a BIR-owned producer-kind vocabulary for Route 1.
- Prototype value and instruction annotations for:
  - producer instruction/index
  - source value/name/type identity
  - immediate integer constants
  - materialization availability
- Preserve prepared producer and constant queries as oracle checks:
  - `find_prepared_same_block_scalar_producer`
  - `evaluate_prepared_same_block_integer_constant`
- Add only cheap lookup indexes that reference typed annotation records.

## Non-Goals

- Do not copy `PreparedSameBlockScalarProducer` or
  `PreparedSameBlockValueMaterializationQuery` as the schema shape.
- Do not move register allocation, storage, frame layout, spill/reload,
  operand-view, publication hook, or final instruction ordering facts into BIR.
- Do not switch broad MIR/AArch64 emission behavior before BIR/prepared query
  equivalence is proven.
- Do not downgrade supported tests, weaken expectations, or add named-case
  shortcuts.

## Working Model

Producer identity should be represented by typed BIR records attached to values
and producer instructions. Function-local lookup structures may keep
same-block query cost low, but those indexes should reference annotation
records and be rebuildable or checkable from the records. Prepared query
helpers remain available until the BIR-backed answers match the accepted
semantic subset.

## Execution Rules

- Keep each implementation step behavior-preserving until the explicit consumer
  switch step.
- Prefer adding BIR query APIs and oracle tests before editing production
  consumers.
- Treat mismatches between BIR and prepared answers as blockers, not as
  expectation-rewrite opportunities.
- Use representative producer coverage: immediate, load-local, load-global,
  cast, binary, select-materialization, future-producer fail-closed, and
  missing-fact fail-closed cases.
- Run a build or compile proof for every code-changing packet, then the narrow
  producer-query subset. Escalate if a packet touches shared BIR or backend
  query behavior beyond Route 1.

## Ordered Steps

### Step 1. Inventory Producer Identity Surfaces

Goal: identify the exact current Route 1 producer-kind, query, and test
surfaces before introducing the BIR-owned schema.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect existing BIR producer-like enums and query records, including
  comparison and call-boundary producer kinds, to avoid duplicate or conflicting
  vocabulary.
- Inspect prepared producer-kind authority in
  `PreparedEdgePublicationSourceProducerKind` and how
  `PreparedSameBlockScalarProducer` is populated.
- Identify where same-block integer constants and materialization availability
  are currently computed.
- Record the proposed BIR schema surfaces, oracle helpers, and narrow proof
  command in `todo.md` without editing source ideas.

Completion check:

- `todo.md` names the concrete BIR/prepared files to edit next, the producer
  kinds that need equivalence coverage, and the narrow test command the
  executor should use for the first code-changing packet.

### Step 2. Define The BIR Producer Vocabulary And Records

Goal: add the target-neutral BIR producer-kind vocabulary and typed annotation
records needed by same-block producer and constant queries.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`

Actions:

- Add or consolidate a Route 1 BIR-owned producer-kind vocabulary that is not
  owned by `PreparedEdgePublicationSourceProducerKind`.
- Define typed value/instruction producer identity records for value/name/type,
  producer instruction/index, immediate constants, and materialization
  availability.
- Keep field names target-neutral and BIR-id based.
- Avoid storage homes, registers, frame slots, emitted availability, and final
  instruction data.

Completion check:

- The project builds or compiles the touched BIR/backend targets.
- The new records are type-checked and have no production consumer switch yet.

### Step 3. Add BIR Same-Block Producer Queries And Indexes

Goal: expose BIR-owned query APIs and cheap lookup indexes for Route 1 while
keeping prepared answers as the oracle.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- existing backend query or prealloc bridge files identified in Step 1

Actions:

- Build or populate function-local lookup indexes that point at typed BIR
  producer identity records.
- Add BIR query APIs for same-block scalar producer, integer constant, and
  materialization availability facts.
- Ensure future producers and missing facts fail closed, matching prepared
  query semantics.
- Keep aggregate lookup facades private; do not introduce a BIR-owned
  `PreparedFunctionLookups` clone.

Completion check:

- BIR queries can answer the Route 1 semantic subset without calling prepared
  producer-kind authority as the source of truth.
- Narrow tests compare BIR and prepared query answers for positive and negative
  cases.

### Step 4. Prove Oracle Equivalence

Goal: demonstrate that BIR-backed Route 1 answers match the old prepared query
answers before any consumer switch.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- any existing narrow backend/codegen tests identified in Step 1

Actions:

- Add or extend equivalence checks against
  `find_prepared_same_block_scalar_producer`.
- Add or extend equivalence checks against
  `evaluate_prepared_same_block_integer_constant`.
- Cover each supported producer kind and negative fail-closed shape.
- Preserve existing prepared query behavior as the comparison oracle.

Completion check:

- Matching before/after narrow proof logs exist for the selected backend/BIR
  subset.
- No expectations are downgraded or weakened.

### Step 5. Switch One Low-Risk Identity Consumer

Goal: switch at most one low-risk consumer that only needs producer/value
identity after BIR/prepared equivalence is proven.

Primary targets:

- the consumer identified in Step 1 as needing only Route 1 semantic identity

Actions:

- Replace a prepared-only read with the BIR-backed query where no storage,
  register, frame, publication hook, or final emission data is needed.
- Keep the prepared query available as an oracle or fallback during migration.
- Avoid broad MIR/AArch64 emission rewrites.

Completion check:

- The narrow producer-query subset remains green.
- The consumer-specific backend/codegen subset remains green.
- Any broader validation requested by the supervisor passes before closure.

### Step 6. Closure Review And Follow-Up Triage

Goal: decide whether the source idea is complete or whether a smaller follow-up
idea is needed for remaining Route 1 consumers.

Actions:

- Verify the schema owns producer/source identity through typed BIR records and
  query APIs.
- Verify prepared query oracles remain available where consumers have not yet
  switched.
- Record any remaining non-Route-1 or target-policy work as separate ideas,
  not as scope expansion.

Completion check:

- The active runbook has proof for schema definition, query equivalence, and
  any permitted consumer switch.
- The reviewer has enough evidence to reject overfit, expectation downgrades,
  or target-policy leakage.
