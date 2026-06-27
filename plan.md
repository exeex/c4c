# Pointer Base-Plus-Offset Materialization Runbook

Status: Active
Source Idea: ideas/open/415_prepared_value_materialization_contracts.md

## Purpose

Continue the prepared value materialization contract work after completing the
rematerializable integer immediate family.

## Goal

`PreparedValueHomeKind::PointerBasePlusOffset` values must expose explicit
prepared materialization facts, producer-side verifier reports, and selected
target consumers that fail closed instead of reconstructing pointer arithmetic
from raw BIR shape.

## Core Rule

Do not add target-local evaluators for arbitrary pointer expressions. Migrate
only pointer base-plus-offset facts that name producer-owned base identity,
byte delta, value/home identity, and target scheduling authority.

## Read First

- `ideas/open/415_prepared_value_materialization_contracts.md`
- `docs/prepared_fact_contracts/value_materialization_contract_plan.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_contract_verifier.*`
- RV64/AArch64 helpers that consume `PointerBasePlusOffset` homes or nearby
  pointer arithmetic producers

## Current Targets

- `PreparedValueHomeKind::PointerBasePlusOffset`
- pointer base value/symbol identity in `PreparedValueHome`
- pointer byte delta facts
- selected RV64/AArch64 pointer materialization consumers
- contract-plan documentation under
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`

## Non-Goals

- Do not rewrite BIR pointer semantics.
- Do not broaden general local-memory lowering unrelated to pointer
  materialization facts.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Do not add filename-specific fixes for torture representatives.

## Working Model

- A pointer base-plus-offset fact needs destination value identity, base value
  or symbol identity, byte delta, and a clear target admission rule.
- A missing base identity is producer-missing.
- Conflicting base payloads or cross-family immediate/materialization payloads
  are producer-incoherent.
- Target consumers may schedule pointer materialization only from coherent
  prepared facts.

## Execution Rules

- Track routine progress in `todo.md`; edit this runbook only for true route
  changes.
- Keep the rematerializable integer immediate work as completed context, not as
  a reason to expand this runbook.
- For each code-changing step, run the supervisor-selected build plus focused
  tests. Run default CTest at the broad-validation checkpoint.
- Treat diagnostics-only changes that leave target pointer recovery intact as
  incomplete.

## Step 1: Map Pointer Materialization Boundaries

Goal: identify producer facts and target consumers for
`PointerBasePlusOffset`.

Primary targets: value-home records, RV64/AArch64 pointer materialization
helpers, and current verifier/report surfaces.

Actions:

- Inspect current `PointerBasePlusOffset` producers in prealloc.
- Identify where RV64/AArch64 consumers read `pointer_base_value_name`,
  `pointer_base_symbol_name`, or `pointer_byte_delta`.
- Record required facts for base identity, symbol identity, byte delta,
  source value identity, and target admission.
- Choose one bounded consumer migration target.

Completion check:

- `todo.md` names required pointer facts, rejected combinations, and the
  focused proof command for Step 2.

## Step 2: Add Typed Pointer Materialization Fact

Goal: expose one explicit prepared pointer base-plus-offset materialization
fact without broadening target recovery.

Actions:

- Add the typed fact payload and compatibility query for coherent
  `PointerBasePlusOffset` homes.
- Reject missing base identity, missing byte delta, missing function/value
  names, and cross-family payloads.
- Update
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`.
- Add focused fact-query tests.

Completion check:

- The pointer family has a typed query that exposes only coherent facts.

## Step 3: Add Producer-Side Verification

Goal: classify missing or contradictory pointer materialization facts before
target consumers attempt recovery.

Actions:

- Add verifier statuses and reports for the pointer fact.
- Map missing facts to `producer_missing`.
- Map contradictory base/value/symbol or cross-family payloads to
  `producer_incoherent`.
- Add focused verifier tests.

Completion check:

- Missing and contradictory pointer facts fail closed with precise statuses.

## Step 4: Migrate Selected Pointer Consumer

Goal: make one selected target pointer materialization consumer use the typed
pointer fact.

Actions:

- Replace direct value-home payload reads or nearby BIR pointer-shape recovery
  with the typed query plus verifier checks.
- Preserve valid prepared scheduling behavior.
- Add or update focused RV64/AArch64 tests for the migrated consumer.

Completion check:

- The migrated consumer no longer reconstructs the selected pointer
  materialization family from raw BIR shape when facts are missing.

## Step 5: Broaden Validation and Decide Remaining Producer Chains

Goal: prove the pointer migration did not regress normal CTest and decide
whether same-block producer-chain materialization needs another follow-up
runbook.

Actions:

- Run build plus default `ctest --test-dir build -j --output-on-failure`.
- Inspect RV64 object/runtime and gcc_torture movement.
- Record the next producer-chain family or lifecycle-review request in
  `todo.md`.

Completion check:

- Default CTest does not regress.
- `todo.md` identifies the next producer-chain family or requests lifecycle
  review.
