# Prepared Storage Layout And Initializer Contracts

Status: Open
Type: Follow-up contract refactor idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Depends On: initial taxonomy from `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md` and target-consumer findings from `ideas/open/418_prepared_target_consumer_boundary_audit.md`

## Goal

Normalize local/global storage layout, memory-access, and initializer facts so
targets consume explicit prepared bytes/ranges/relocations instead of
reconstructing source layout.

## Why This Exists

Idea 405 repaired contradictory local aggregate/union slot extents before RV64
local-memory lowering could proceed. Idea 409 repaired packed/fp128 global
initializer admission before RV64 object emission. Current prepared addressing
publishes base, offset, size, alignment, and provenance, but global object
emission still inspects BIR globals directly for simple storage shapes.

## In Scope

- Define normalized storage object facts for local slots: extent, alignment,
  byte range, overlay/alias authority, and memory provenance.
- Use the early target-consumer audit to identify RV64/AArch64 storage or
  object-data paths that still inspect raw BIR/global structure to recover
  producer-owned layout or initializer facts.
- Define normalized global initializer/storage facts: emitted bytes,
  relocations, zero-fill, symbol references, and precise unsupported outcomes.
- Add verifier checks for contradictory slot ranges and incomplete initializer
  facts.
- Migrate one local-memory and one global-data consumer path to the normalized
  facts.
- Add focused tests for aggregate/union slot layout and packed/fp128/global
  initializer admission.

## Out Of Scope

- Having RV64 synthesize C aggregate/union layout or packed initializer bytes.
- Rewriting the frontend type system.
- Broad ELF/data-section rewrites not backed by prepared facts.
- Treating RV64 gcc_torture pass count as the non-regression harness.

## Acceptance Criteria

- Selected local and global storage facts are coherent before target consumers
  run, or fail with producer-owned diagnostics.
- Migrated target consumers no longer recover selected layout/initializer
  semantics from source type text or BIR global structure.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- RV64 gcc_torture may temporarily lose passes only where the new contract
  fail-closes previously ambiguous storage facts.

## Reviewer Reject Signals

- Reject RV64/MIR code that fabricates slot size, aliasing, aggregate layout,
  initializer bytes, or relocations not present in prepared facts.
- Reject merely widening all local slots or accepting all globals without
  preserving range/alignment/provenance.
- Reject filename-specific fixes for `20020225-2.c`, `20040709-2.c`, or other
  named torture representatives.
- Reject diagnostics-only churn that leaves contradictory one-byte aggregate
  slots or unsupported initializer forms reaching targets.
- Reject expectation rewrites, allowlist filtering, or unsupported downgrades.
