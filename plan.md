# BIR Comparison And Condition Annotation Schema Plan

Status: Active
Source Idea: ideas/open/165_bir_comparison_condition_annotation_schema.md

## Purpose

Move Phase A Route 7 comparison and materialized-condition producer facts into
BIR-owned instruction and terminator annotations while preserving prepared
fused-operand and materialized-condition queries as migration oracles.

## Goal

Consumers can answer comparison operand provenance, condition value provenance,
fused-operand provenance, materialized-condition producer identity, and
branch-condition provenance from typed BIR annotations without importing target
compare/branch lowering policy.

## Core Rule

Keep comparison and condition annotations semantic and target-neutral. Do not
encode AArch64 condition codes, fused-compare legality decisions, branch
spelling, hazard handling, emitted-register state, final instruction records or
errors, final instruction order, or target compare/branch instruction
selection in BIR annotations.

## Read First

- `ideas/open/165_bir_comparison_condition_annotation_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR instruction annotations for comparison-producing `BinaryInst`
  provenance, condition value name, lhs/rhs producer nodes or integer
  constants, producer instruction index, fused-operand provenance, and
  materialized-condition producer references.
- BIR terminator annotations for branch-condition provenance that point to
  comparison or materialized-condition producer records through stable BIR ids.
- Function-local lookup/index helpers only where needed for cheap comparison
  and condition answers over instruction/terminator annotations.
- Prepared fused-compare operand and materialized-condition producer queries as
  temporary oracle checks until BIR-backed answers are proven equivalent.

## Non-Goals

- Do not migrate fused-compare legality, condition-code selection, branch
  emission strategy, final instruction records/errors, hazard handling,
  emitted-register state, final instruction order, or AArch64 compare/branch
  instruction selection.
- Do not preserve whole `PreparedFusedCompareOperandProducer` or
  `PreparedMaterializedConditionProducer` records as the BIR annotation schema.
- Do not weaken materialized-condition, integer-constant operand,
  non-comparison, absent-provenance, or unsupported-path contracts.
- Do not switch target/codegen branch lowering or final-emission consumers
  before BIR and prepared answers are proven equivalent on accepted positive
  and negative cases.

## Working Model

- Instruction annotations own semantic comparison producer and operand
  provenance facts.
- Terminator annotations own branch-condition provenance by referring to BIR
  comparison or materialized-condition producer records, not target condition
  code names.
- Lookup helpers may be rebuilt from BIR instruction/terminator annotations,
  but they are indexes, not the semantic source of truth.
- Missing or non-materializable provenance must remain explicit enough that
  consumers do not infer target lowering policy.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Use existing prepared fused-compare operand and materialized-condition
  producer answers as oracle checks until a migration step explicitly switches
  a consumer to BIR-backed answers.
- Add negative coverage alongside positive comparison/condition coverage,
  including materialized condition, integer-constant operand, non-comparison,
  missing producer, absent provenance, and non-materializable cases where
  applicable.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because shared
  comparison/condition query helpers can affect both x86 and AArch64 backend
  callers.

## Steps

### Step 1: Inventory Comparison And Condition Surfaces

Goal: identify the exact prepared comparison/condition query surfaces, BIR
payload targets, and proof tests before adding schema code.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect prepared fused-compare operand producer and materialized-condition
  producer query records and helper APIs.
- Record concrete instruction-annotation fields, terminator-annotation fields,
  optional function-local index shape, positive oracle cases, negative oracle
  cases, and the narrow proof command in `todo.md`.
- Identify the first low-risk test target for schema introduction.

Completion check:

- `todo.md` names the concrete schema fields, oracle helpers, expected
  materialized-condition/integer-constant/non-comparison/absent-provenance
  cases, and delegated proof command for Step 2.

### Step 2: Add BIR Annotation Records

Goal: add typed BIR annotation records for Route 7 comparison and condition
facts without switching production consumers.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add instruction annotation types for comparison-producing `BinaryInst`
  provenance, condition value name, lhs/rhs producer nodes or integer
  constants, producer instruction index, fused-operand provenance, and
  materialized-condition producer references.
- Add terminator annotation types for branch-condition provenance that refer to
  BIR comparison/materialized-condition records by stable BIR ids.
- Add construction/query helpers that distinguish absent or
  non-materializable provenance from missing annotation state.
- Add oracle test coverage that compares BIR annotation answers with existing
  prepared comparison/condition answers for accepted positive and negative
  cases.

Completion check:

- Narrow backend tests prove the new BIR annotations are populated and match
  prepared comparison/condition oracles without switching broad consumers.

### Step 3: Add Lookup/Index Helpers

Goal: provide stable BIR-id-based lookup helpers over the Step 2 annotations.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add function-local lookup/index helpers only where linear scans would make
  migration awkward for fused-operand and materialized-condition producer
  queries.
- Keep the index rebuildable from BIR instruction and terminator annotation
  payloads.
- Prove fused-operand success, materialized-condition success, integer
  constant operand, non-comparison, missing producer, absent provenance, and
  non-materializable behavior.

Completion check:

- Tests demonstrate that lookup helpers are target-neutral indexes over BIR
  annotation payloads and fail closed for explicit negative cases.

### Step 4: Migrate A Low-Risk Query Consumer

Goal: switch the narrowest shared query helper to read BIR-backed Route 7 facts
while retaining prepared comparison/condition oracle checks.

Primary targets:

- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- backend tests identified in Step 1

Actions:

- Replace one low-risk prepared-owned semantic comparison or condition answer
  with a BIR-backed annotation lookup.
- Keep prepared comparison/condition results in tests as equivalence oracles
  during the migration.
- Avoid target branch lowering, final instruction records, emitted-register
  state, or AArch64 compare/branch call sites unless the broader shared query
  layer is already proven.

Completion check:

- Narrow tests prove BIR-backed and prepared answers match for positive,
  materialized-condition, integer-constant, non-comparison, missing producer,
  absent-provenance, non-materializable, and no-match cases selected in Step 1.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the Route 7 schema is complete enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor,
  including AArch64-relevant coverage if shared query helpers changed.
- Confirm no expectation downgrades, target branch-policy leaks, whole
  prepared record copies, target/codegen rewrites, or helper-only reshuffles
  remain.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
