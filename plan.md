# BIR Comparison Condition Producer Identity Runbook

Status: Active
Source Idea: ideas/open/158_bir_comparison_condition_producer_identity.md

## Purpose

Move target-neutral comparison and materialized-condition producer identity
toward BIR ownership while preserving AArch64 fused-compare legality,
condition-code selection, branch emission, and final instruction policy in
prealloc/codegen.

## Goal

Add BIR-owned comparison producer queries that can match prepared fused-operand
and materialized-condition producer answers before any comparison or branch
consumer switches to BIR provenance reads.

## Core Rule

BIR may own semantic producer identity for comparisons and condition values:
lhs/rhs producer nodes or integer constants, comparison-producing `BinaryInst`,
condition value name, and producer instruction index. Do not move fused-compare
legality, condition-code emission, branch strategy, final instruction records,
hazard handling, emitted-register state, or AArch64 compare/branch instruction
selection into BIR.

## Read First

- `ideas/open/158_bir_comparison_condition_producer_identity.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- prepared fused-compare operand producer helpers under `src/backend/prealloc/`
- prepared materialized-condition producer helpers under `src/backend/prealloc/`
- BIR comparison, branch, and value metadata surfaces under `src/backend/bir/`
- AArch64 compare/branch consumers under `src/backend/mir/aarch64/`
- backend BIR prepared-lookup tests and nearby comparison/branch case tests

## Current Targets

- Prepared oracle surfaces for fused-compare operand producer answers.
- Prepared oracle surfaces for materialized-condition producer answers.
- BIR relationships keyed by branch condition or materialized condition value.
- Comparison records that preserve producer identity for lhs/rhs nodes,
  integer constants, comparison-producing `BinaryInst`, condition value name,
  and producer instruction index.
- Comparison and branch consumers only after BIR answers are proven equivalent
  for the specific provenance read being switched.

## Non-Goals

- Do not change fused-compare legality, condition-code selection, branch
  emission strategy, final instruction records/errors, hazard handling,
  emitted-register state, or AArch64 compare/branch instruction selection.
- Do not treat target branch lowering policy as a BIR-owned fact.
- Do not weaken supported-path tests, rewrite expectations to claim progress,
  or add named-testcase shortcuts.
- Do not switch consumers until prepared-oracle equivalence is proven for the
  relevant constants, same-block producers, binary compare values, and
  unavailable cases.

## Working Model

- Prepared comparison and condition producer helpers remain the oracle until
  BIR answers match one provenance family at a time.
- BIR owns the semantic relationship: "this branch condition or condition
  value was produced by this comparison and these operand producers/constants."
- AArch64 keeps deciding whether a compare can be fused, how condition codes
  map to branches, and how final instructions are emitted.
- Consumer switches are limited to provenance reads. Any diff that changes
  compare/branch target behavior before equivalence proof is route drift.

## Execution Rules

- Work in narrow, observable steps. Add the query surface and bridge before
  switching any consumer.
- For each query, compare old prepared answers and new BIR answers before
  treating BIR as authority.
- Include positive and negative coverage for integer constants, same-block
  producers, binary compare values, materialized condition values, and
  unavailable/non-fusable paths.
- Keep target lowering behavior byte-for-byte or semantically unchanged unless
  the supervisor delegates a separate target-lowering change.
- If implementation requires moving branch fusion legality, condition-code
  emission, hazard handling, emitted-register tracking, or final instruction
  records into BIR, stop and escalate for route review.
- Use matching before/after backend comparison and branch coverage when the
  supervisor delegates proof. Escalate to broader backend validation if shared
  BIR relationship infrastructure or AArch64 compare/branch lowering changes.

## Ordered Steps

### Step 1: Inspect the prepared comparison producer oracle

Goal: define the semantic comparison producer boundary before adding BIR state.

Primary target: prepared fused-operand and materialized-condition producer
helpers plus existing comparison/branch tests.

Actions:

- Inspect prepared fused-compare operand producer lookup helpers and their
  returned fields.
- Inspect prepared materialized-condition producer lookup helpers and their
  returned fields.
- Classify fields into BIR-owned semantic producer identity versus AArch64
  legality, condition-code, branch emission, or final-instruction policy.
- Identify existing coverage for integer constants, same-block producers,
  binary compare values, materialized condition values, unavailable producers,
  and non-fusable paths.
- Record any missing narrow coverage in `todo.md` before implementing code.

Completion check:

- The executor can name the exact comparison/condition producer facts to model
  in BIR and the exact prepared or AArch64 fields that must remain outside BIR.

### Step 2: Add the BIR comparison producer relationship surface

Goal: expose a BIR-owned query shape for comparison and materialized-condition
producer identity without changing branch lowering behavior.

Primary target: BIR relationship or analysis-cache code adjacent to existing
BIR function/block/value metadata.

Actions:

- Add a target-neutral comparison producer record keyed by branch condition or
  condition value.
- Store lhs/rhs producer nodes or integer constants where available.
- Store the comparison-producing `BinaryInst`, condition value name, and
  producer instruction index.
- Make unavailable or ambiguous producers fail closed instead of inventing
  target-specific answers.
- Add query tests using BIR-level fixtures without switching production
  comparison or branch consumers.

Completion check:

- The project builds, and tests can query BIR comparison producer identity
  without changing AArch64 compare/branch selection or emission behavior.

### Step 3: Bridge prepared answers to BIR comparison queries

Goal: prove the BIR query can represent the semantic facts currently provided
by prepared fused-operand and materialized-condition producer helpers.

Primary target: backend BIR prepared-lookup tests and comparison/branch
coverage.

Actions:

- Add equivalence checks for fused-compare operand producer answers.
- Add equivalence checks for materialized-condition producer answers.
- Cover integer constants, same-block producers, binary compare values,
  condition value names, producer instruction indexes, and unavailable cases.
- Include non-fusable negative paths to prove BIR provenance does not imply
  branch fusion legality.
- Keep prepared helpers as the source of truth during this step.

Completion check:

- Narrow backend coverage proves BIR comparison producer answers match prepared
  semantic producer answers for the accepted fact families while target
  compare/branch behavior remains unchanged.

### Step 4: Populate or extract BIR facts from production comparison paths

Goal: make comparison producer identity available from normal production BIR
or from a production extraction/analysis path.

Primary target: production BIR construction, comparison analysis, and prepared
lookup bridge code.

Actions:

- Inspect production paths that create comparison `BinaryInst` values,
  materialized condition values, and branch conditions.
- Populate the BIR comparison producer relationship during construction, or
  add a production analysis path that derives equivalent semantic facts from
  existing BIR.
- Prove the bridge on normal lowered comparison/branch cases, not only
  hand-built fixtures.
- Keep prepared helpers as the oracle and do not switch AArch64/prealloc
  consumers in this step.

Completion check:

- Normal production BIR contains or can extract the comparison producer facts
  needed by fused-operand and materialized-condition producer queries.
- Narrow backend coverage proves production-populated or extracted facts match
  the prepared semantic oracle.

### Step 5: Switch comparison and branch provenance consumers

Goal: read comparison producer provenance from BIR only after equivalence is
proven for the specific consumer.

Primary target: comparison/branch consumers that currently use prepared
producer provenance only.

Actions:

- Switch one provenance consumer at a time from prepared producer helpers to
  BIR comparison producer queries.
- Keep fused-compare legality, condition-code selection, branch emission, final
  instruction records, hazards, and emitted-register state on their existing
  AArch64/prealloc authority.
- Preserve prepared-oracle comparison coverage during each switch.
- Add focused negative coverage that proves unavailable BIR producer facts
  fail closed without changing target branch behavior.

Completion check:

- Comparison and branch consumers read BIR producer provenance for the migrated
  fact family only, with prepared-equivalence tests still green and no target
  branch policy moved into BIR.

### Step 6: Run acceptance validation and route review check

Goal: prove the idea's acceptance criteria without relying on a single narrow
case.

Primary target: matching before/after backend comparison and branch coverage
chosen by the supervisor.

Actions:

- Run the supervisor-delegated build and backend comparison/branch test subset
  with canonical before/after logs.
- Include equivalence checks for constants, same-block producers, binary
  compare values, materialized conditions, and unavailable/non-fusable paths.
- Inspect the final diff for expectation rewrites, named-case shortcuts, or
  target policy movement into BIR.
- Escalate to reviewer scrutiny if proof is narrow, policy moved layers, or
  nearby same-feature cases remain unexamined.

Completion check:

- The source idea acceptance criteria are met: BIR comparison queries match
  prepared fused-operand and materialized-condition producer answers, and
  comparison/branch consumer switches are limited to producer provenance.
