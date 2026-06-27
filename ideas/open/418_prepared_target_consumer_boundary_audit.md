# Prepared Target Consumer Boundary Audit

Status: Open
Type: Follow-up audit and cleanup idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Runs After: `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md`
Feeds: `ideas/open/414_typed_prepared_call_argument_contracts.md`, `ideas/open/415_prepared_value_materialization_contracts.md`, `ideas/open/416_prepared_helper_operand_home_contracts.md`, `ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`

## Goal

Audit RV64 and AArch64 prepared consumers for producer-fact recovery,
semantic reconstruction, and target-local layout or ABI guessing.

This audit should run immediately after the first verifier/taxonomy slice so
the typed contract ideas can use concrete target-consumer findings instead of
choosing scopes from suspicion alone.

## Why This Exists

Idea 412 found that many target paths are legitimate coherent-fact lowering, but
some helpers search raw BIR blocks, globals, or source spellings to recover
information that should be a prepared fact. These helpers need classification
before more gcc_torture-driven repairs add hidden target fixups.

## In Scope

- Inventory target prepared consumers that inspect raw BIR instructions,
  globals, type text, source spellings, or fallback names beyond simple
  identity lookup.
- Classify each finding as:
  - acceptable coherent-fact lowering,
  - target scheduling over explicit prepared facts,
  - producer contract gap requiring a separate idea,
  - semantic BIR gap requiring a separate idea.
- Remove or quarantine one low-risk target-side recovery path if an existing
  prepared fact already covers it.
- Produce a short review artifact listing remaining findings and next owners.

## Out Of Scope

- Rewriting all RV64/AArch64 lowering.
- Masking producer defects in target code.
- Changing default CTest selection or adding RV64 gcc_torture to default
  harness.

## Acceptance Criteria

- A review artifact lists audited target helpers with owner classification and
  file references.
- At least one concrete cleanup or follow-up owner decision is made.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- RV64 gcc_torture pass count may temporarily regress only when target recovery
  is replaced by explicit fail-closed producer diagnostics.

## Reviewer Reject Signals

- Reject an audit that only says "target code is messy" without naming helpers,
  files, and owner classifications.
- Reject deleting target support without a precise producer diagnostic or
  migrated prepared fact.
- Reject RV64/MIR changes that infer missing ABI homes, helper addresses,
  materialized constants, aggregate layout, or initializer bytes.
- Reject broad rewrites outside prepared consumer boundaries.
- Reject expectation rewrites, allowlist filtering, or weaker runtime checks.
