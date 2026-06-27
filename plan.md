# Prepared Value Materialization Contracts Runbook

Status: Active
Source Idea: ideas/open/415_prepared_value_materialization_contracts.md

## Purpose

Continue moving target-required producer knowledge out of target recovery code
and into explicit prepared materialization facts.

## Goal

Wide rematerialized immediates, pointer base-plus-offset values, and selected
same-block producer chains must be represented as prepared facts or fail
closed before target consumers reconstruct raw BIR expression semantics.

## Core Rule

Do not add target-local evaluators for arbitrary BIR expressions. Migrate only
materialization families that can be described by explicit prepared facts with
producer-owned identity, width, signedness, and scheduling authority.

## Read First

- `ideas/open/415_prepared_value_materialization_contracts.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_contract_verifier.*`
- RV64/AArch64 materialization helpers that currently inspect nearby BIR
  producers

## Current Targets

- `PreparedValueHomeKind::RematerializableImmediate`
- `PreparedValueHomeKind::PointerBasePlusOffset`
- selected same-block producer-chain materialization helpers
- verifier/report surfaces in `src/backend/prealloc/prepared_contract_verifier.*`
- contract-plan documentation under
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`

## Non-Goals

- Do not rewrite BIR semantics.
- Do not broaden RV64/AArch64 scalar lowering unrelated to materialization
  facts.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Do not add filename-specific fixes for torture representatives.

## Working Model

- Rematerialized immediates need explicit width, signedness, value identity,
  and target-legal range facts.
- Pointer base-plus-offset values need base identity, byte delta, value/home
  identity, and provenance that the target may schedule.
- Same-block producer chains may stay schedulable only when prepared facts name
  the producer relation and target-required operands.

## Execution Rules

- Track routine progress in `todo.md`; edit this runbook only for true route
  changes.
- Cite consumed taxonomy/audit rows in the value materialization contract plan.
- For each code-changing step, run the supervisor-selected build plus focused
  tests. Run default CTest at the broad-validation checkpoint.
- Treat diagnostics-only changes that leave target recovery intact as
  incomplete.

## Step 1: Map Current Materialization Recovery Boundaries

Goal: identify the concrete producer facts and target consumers for the first
materialization family.

Primary targets: value-home records, RV64/AArch64 materialization helpers, and
current verifier/report surfaces.

Actions:

- Inspect `RematerializableImmediate`, `PointerBasePlusOffset`, and same-block
  producer-chain paths.
- Record required facts for width, signedness, source identity, byte deltas,
  and scheduling authority.
- Identify which RV64/AArch64 consumers currently infer materialization
  semantics from raw BIR shape.
- Choose the first bounded materialization family for migration.

Completion check:

- `todo.md` names the first selected family, required facts, rejected
  combinations, and focused proof command for Step 2.

## Step 2: Add the First Typed Materialization Fact

Goal: expose one explicit prepared materialization fact without broadening
target recovery.

Actions:

- Add the typed fact payload and compatibility query for the selected family.
- Reject missing width/signedness/source/provenance facts.
- Create or update
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`.
- Add focused fact-query tests.

Completion check:

- The selected family has a typed query that exposes only coherent facts.

## Step 3: Add Producer-Side Verification

Goal: classify missing or contradictory materialization facts before target
consumers attempt recovery.

Actions:

- Add verifier statuses and reports for the selected family.
- Map missing facts to `producer_missing`.
- Map contradictory facts or cross-family payloads to `producer_incoherent`.
- Add focused verifier tests.

Completion check:

- Missing and contradictory facts fail closed with precise statuses.

## Step 4: Migrate Selected Consumers

Goal: make one or two target consumers use the typed materialization fact.

Actions:

- Replace raw BIR producer-shape recovery with the typed query plus verifier
  checks.
- Preserve valid prepared scheduling behavior.
- Add or update focused RV64/AArch64 tests for the migrated consumer.

Completion check:

- Migrated consumers no longer reconstruct the selected materialization family
  from raw BIR shape when facts are missing.

## Step 5: Broaden Validation and Decide Next Family

Goal: prove the migrated family did not regress normal CTest and decide whether
another materialization family needs a follow-up runbook.

Actions:

- Run build plus default `ctest --test-dir build -j --output-on-failure`.
- Inspect any RV64 gcc_torture movement.
- Record the next family or lifecycle-review request in `todo.md`.

Completion check:

- Default CTest does not regress.
- `todo.md` identifies the next materialization family or requests lifecycle
  review.
