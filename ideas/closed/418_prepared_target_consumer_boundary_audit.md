# Prepared Target Consumer Boundary Audit

Status: Closed
Type: Follow-up audit and cleanup idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Runs After: `ideas/closed/413_prepared_contract_verifier_and_owner_taxonomy.md`
Feeds: `ideas/closed/414_typed_prepared_call_argument_contracts.md`, `ideas/closed/415_prepared_value_materialization_contracts.md`, `ideas/closed/416_prepared_helper_operand_home_contracts.md`, `ideas/closed/417_prepared_storage_layout_and_initializer_contracts.md`

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
- Consume `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
  and classify findings using the idea 413 taxonomy rather than inventing a
  separate target-local vocabulary.
- Classify each finding as:
  - acceptable coherent-fact lowering,
  - target scheduling over explicit prepared facts,
  - producer contract gap requiring a separate idea,
  - semantic BIR gap requiring a separate idea.
- Remove or quarantine one low-risk target-side recovery path if an existing
  prepared fact already covers it.
- Produce `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
  listing remaining findings, row IDs, consumed taxonomy rows, and next owners.

## Out Of Scope

- Rewriting all RV64/AArch64 lowering.
- Masking producer defects in target code.
- Changing default CTest selection or adding RV64 gcc_torture to default
  harness.

## Acceptance Criteria

- The target-consumer audit artifact lists audited target helpers with owner
  classification, file references, consumed taxonomy rows, and row IDs for
  downstream 414-417 handoff.
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

## Completion Note

Closed after Step 5 close-readiness. The published audit artifact is
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md`; it lists
RV64/RISC-V and AArch64 target consumer rows, consumed taxonomy rows, owner
classifications, current and required behavior, and downstream handoff rows for
ideas 414 through 417.

The required concrete cleanup or owner decision was satisfied by the Step 3
decision for `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` and
`418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`: idea 417 owns RV64 object/global
storage, initializer bytes, sections, relocations, labels, publication
identity, and memory-access provenance; idea 415 owns only the
symbol/value-materialization fallback portion of the RV64 global-memory helper
row. The Step 4 handoff section preserves that split, and records that idea 416
has no selected recovery row from this audit.

Close proof: supervisor-recorded default
`ctest --test-dir build -j --output-on-failure` in `test_after.log` passed
`3355/3355`. Earlier backend subset proof covered `326/326` tests. Because the
available canonical logs cover different scopes, this closure does not claim a
matching-scope before/after regression-guard comparison or PASS over those
different-scope logs. No testcase-overfit, expectation weakening, target
fallback inference, or broad rewrite outside the prepared consumer boundary
was present in the completed audit route.
