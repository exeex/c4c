# Same-Block Producer-Chain Materialization Runbook

Status: Active
Source Idea: ideas/open/415_prepared_value_materialization_contracts.md

## Purpose

Continue the prepared value materialization contract work after completing the
rematerializable integer immediate and pointer base-plus-offset families.

## Goal

Selected same-block producer chains must expose explicit prepared
materialization facts, producer-side verifier reports, and migrated target
consumers that fail closed instead of rediscovering raw BIR producer shape.

## Core Rule

Do not add target-local evaluators for arbitrary BIR expressions. Migrate only
bounded same-block producer chains that can name producer identity, operand
facts, scheduling authority, and target admission.

## Read First

- `ideas/open/415_prepared_value_materialization_contracts.md`
- `docs/prepared_fact_contracts/value_materialization_contract_plan.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_contract_verifier.*`
- RV64/AArch64 helpers that search same-block producers for select, compare,
  binary, or call-argument materialization

## Current Targets

- selected same-block producer-chain materialization helpers
- producer relation identity and target scheduling authority
- operand materialization facts required by the selected chain
- selected RV64/AArch64 producer-chain consumers
- contract-plan documentation under
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`

## Non-Goals

- Do not rewrite BIR semantics.
- Do not broaden scalar/ALU lowering unrelated to prepared materialization
  facts.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Do not add filename-specific fixes for torture representatives.

## Working Model

- A same-block producer-chain fact needs destination value identity, producer
  instruction identity, operand identities, operation kind, and target
  scheduling authority.
- Missing producer relation or operand facts are producer-missing.
- Contradictory producer kind, cross-family payloads, or ambiguous producer
  matches are producer-incoherent.
- Target consumers may schedule a selected producer chain only from coherent
  prepared facts.

## Execution Rules

- Track routine progress in `todo.md`; edit this runbook only for true route
  changes.
- Keep the immediate and pointer work as completed context, not as a reason to
  expand this runbook.
- For each code-changing step, run the supervisor-selected build plus focused
  tests. Run default CTest at the broad-validation checkpoint.
- Treat diagnostics-only changes that leave target producer-shape recovery
  intact as incomplete.

## Step 1: Map Producer-Chain Boundaries

Goal: identify one bounded same-block producer-chain family and its current
target consumers.

Primary targets: RV64/AArch64 helpers that search nearby BIR producers,
producer lookup records, and current verifier/report surfaces.

Actions:

- Inspect current same-block select, compare, binary, and call-argument
  producer recovery paths.
- Identify which paths already have prepared producer lookup records and which
  paths still infer from raw BIR shape.
- Record required facts for producer instruction identity, operand identity,
  operation kind, target admission, and scheduling authority.
- Choose one bounded consumer migration target.

Completion check:

- `todo.md` names the selected producer-chain family, required facts, rejected
  combinations, and focused proof command for Step 2.

## Step 2: Add Typed Producer-Chain Materialization Fact

Goal: expose one explicit prepared producer-chain materialization fact without
broadening target recovery.

Actions:

- Add the typed fact payload and compatibility query for the selected producer
  chain.
- Reject missing producer identity, missing operand facts, missing destination
  identity, ambiguous producer matches, and cross-family payloads.
- Update
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`.
- Add focused fact-query tests.

Completion check:

- The selected producer-chain family has a typed query that exposes only
  coherent facts.

## Step 3: Add Producer-Side Verification

Goal: classify missing or contradictory producer-chain facts before target
consumers attempt recovery.

Actions:

- Add verifier statuses and reports for the selected producer-chain fact.
- Map missing producer/operand/scheduling facts to `producer_missing`.
- Map contradictory or ambiguous producer-chain facts to
  `producer_incoherent`.
- Add focused verifier tests.

Completion check:

- Missing and contradictory producer-chain facts fail closed with precise
  statuses.

## Step 4: Migrate Selected Producer-Chain Consumer

Goal: make one selected target producer-chain materialization consumer use the
typed fact.

Actions:

- Replace raw BIR producer-shape recovery with the typed query plus verifier
  checks.
- Preserve valid prepared scheduling behavior.
- Add or update focused RV64/AArch64 tests for the migrated consumer.

Completion check:

- The migrated consumer no longer reconstructs the selected producer-chain
  family from raw BIR shape when facts are missing.

## Step 5: Broaden Validation and Lifecycle Decision

Goal: prove the producer-chain migration did not regress normal CTest and
decide whether the source idea is complete.

Actions:

- Run build plus default `ctest --test-dir build -j --output-on-failure`.
- Inspect RV64 object/runtime and gcc_torture movement.
- Record whether the source idea can close or needs another follow-up runbook
  in `todo.md`.

Completion check:

- Default CTest does not regress.
- `todo.md` identifies source-idea closure or the next remaining
  materialization family.
