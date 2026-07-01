# Prepared Storage Layout And Initializer Contracts

Status: Closed
Type: Follow-up contract refactor idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Depends On: initial taxonomy from `ideas/closed/413_prepared_contract_verifier_and_owner_taxonomy.md` and target-consumer findings from `ideas/closed/418_prepared_target_consumer_boundary_audit.md`
Handoff Inputs: `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`, `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`

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
- Cite the consumed taxonomy and target-consumer audit rows in
  `docs/prepared_fact_contracts/storage_initializer_contract_plan.md`.
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
- The storage/initializer contract plan names the idea 413/418 rows consumed by
  this slice, or explicitly records that no applicable target-consumer audit
  row exists for the selected storage family.
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

## Completion Note

Closed after Step 6 close-readiness for the selected runbook scope. The
published contract plan is
`docs/prepared_fact_contracts/storage_initializer_contract_plan.md`; it names
the consumed 413 taxonomy rows, 418 audit rows, selected local/global payloads,
classification rules, and Idea 415 materialization boundary.

The implementation migrated one selected local-memory consumer path and one
selected RV64 global/object-data consumer path to producer-owned prepared
facts. Missing or contradictory selected storage/object facts fail closed with
producer-owned classification, and complete but not-yet-lowerable selected
object-data forms are classified as `target_unsupported_but_coherent`.

Selected-scope boundaries remain intentionally outside this closed slice:
string constants stay on the existing path, relocation-bearing globals are
modeled but not lowered from prepared relocation records, pointer/symbol
initializer forms remain `target_unsupported_but_coherent`, and extern global
declarations without initializer storage still bypass selected object-data
verification as declarations. Any follow-up relocation support should publish
producer-owned relocation facts and consume those facts in the target instead
of reconstructing relocations from raw BIR initializer shape.

Close proof: supervisor-accepted full-suite baseline
`test_baseline.log` records commit
`7d3f8d784ae34aebebeb6b2211107183cb302ccb` with `3356/3356` tests passing.
Route review in `review/417_step6_route_review.md` found no blocking overfit,
expectation weakening, allowlist filtering, unsupported downgrades, or
target-side raw layout/initializer reconstruction in the selected paths. The
available canonical logs did not include a matching full-suite `test_after.log`
for a strict before/after regression-guard comparison, and lifecycle close did
not modify test logs.
