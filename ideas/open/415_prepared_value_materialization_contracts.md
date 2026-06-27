# Prepared Value Materialization Contracts

Status: Open
Type: Follow-up contract refactor idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Depends On: initial taxonomy from `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md` and target-consumer findings from `ideas/open/418_prepared_target_consumer_boundary_audit.md`
Handoff Inputs: `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`, `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`

## Goal

Make rematerialized immediates, pointer base-plus-offset values, and producer
chains first-class prepared materialization facts.

## Why This Exists

Idea 404 repaired wide rematerialized immediate admission as a producer contract
boundary. Current value homes include `RematerializableImmediate` and
`PointerBasePlusOffset`, while RV64 still has helper code that searches nearby
BIR producers for select/binary forms. Some producer-chain scheduling is valid,
but it needs an explicit prepared contract so target consumers do not recreate
BIR expression semantics.

## In Scope

- Define materialization fact payloads for immediates, pointer deltas, and
  selected same-block producer chains.
- Use the early target-consumer audit to identify which RV64/AArch64
  materialization helpers are legitimate scheduling over facts versus recovery
  of missing producer facts.
- Cite the consumed taxonomy and target-consumer audit rows in
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md`.
- Add verifier checks that every target-required materialization has complete
  width, signedness, source identity, and scheduling authority.
- Migrate one or two existing RV64 producer-chain consumers to use explicit
  facts.
- Add tests for wide immediates and selected select/binary producer chains.

## Out Of Scope

- Implementing a target-local evaluator for arbitrary BIR expressions.
- Broad scalar/ALU RV64 lowering unrelated to materialization facts.
- Rewriting BIR semantics.
- Weakening runtime comparison or gcc_torture expectations.

## Acceptance Criteria

- Wide rematerialized values and migrated producer chains are represented as
  explicit prepared facts or rejected by a producer-side diagnostic.
- The value-materialization contract plan names the idea 413/418 rows consumed
  by this slice, or explicitly records that no applicable target-consumer audit
  row exists for the selected materialization family.
- Target consumers no longer need to infer migrated materialization semantics
  from raw BIR instruction shape.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- RV64 gcc_torture pass count may temporarily drop only when cases become
  precise fail-closed materialization diagnostics.

## Reviewer Reject Signals

- Reject RV64/MIR code that reconstructs arbitrary BIR constant arithmetic or
  select semantics when prepared materialization facts are missing.
- Reject widening immediate thresholds without preserving width/signedness and
  producer identity.
- Reject filename-specific fixes for `int-compare.c` or runtime-abort
  representatives.
- Reject diagnostics-only changes that leave the same unfactored producer-chain
  recovery in target consumers.
- Reject expectation rewrites, allowlist filtering, or unsupported downgrades.
