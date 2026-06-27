# Prepared Storage Layout And Initializer Contracts Runbook

Status: Active
Source Idea: ideas/open/417_prepared_storage_layout_and_initializer_contracts.md

## Purpose

Normalize selected storage layout, memory-access, global object-data, and
initializer facts so targets consume explicit prepared facts instead of
reconstructing source layout or raw BIR initializer semantics.

## Goal

Move the first RV64 storage/global recovery surfaces from target-side
reconstruction to producer-owned prepared contracts, with verifier
classification and focused proof.

## Core Rule

Storage size, byte ranges, alias/overlay authority, initializer bytes,
relocations, zero-fill, section/publication identity, labels, and memory-access
provenance must come from prepared facts or fail closed. Target code must not
invent those facts from source type text, raw BIR globals, raw names, or nearby
partial prepared fragments.

## Read First

- `ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

## Current Targets And Scope

- Consume taxonomy rows:
  - `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`
  - `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`
  - `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`
  - storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`
- Consume 418 audit rows:
  - `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`
  - `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`
  - storage-relevant reference rows named in the Idea 417 handoff
- Start with one local-memory path and one global/object-data path.
- Keep Idea 415 ownership limited to symbol/value materialization fallback.

## Non-Goals

- Do not rewrite the frontend type system.
- Do not make RV64 synthesize C aggregate/union layout or packed initializer
  bytes locally.
- Do not perform a broad ELF/data-section rewrite unless backed by prepared
  facts for the selected path.
- Do not use RV64 gcc_torture pass count as the acceptance harness.
- Do not weaken expectations, add allowlists, or mark supported cases
  unsupported to claim progress.

## Working Model

- Missing selected storage/object facts are `producer_missing`.
- Contradictory selected slot ranges, byte ranges, labels, sections,
  relocations, initializer bytes, zero-fill, or publication facts are
  `producer_incoherent`.
- Complete but not-yet-lowerable object-data forms are
  `target_unsupported_but_coherent`.
- Invalid pre-prepared initializer semantics remain
  `pre_prepared_semantic_failure`.
- RV64 target emission should become a coherent emitter over complete prepared
  records, not an interpreter for raw initializer semantics.

## Execution Rules

- Preserve existing coherent target lowering while moving selected recovery
  boundaries behind explicit prepared facts.
- Add verifier/report classification before target consumers recover missing
  storage/global facts.
- Keep compatibility bridges narrow and explicit when staging migration.
- Add focused tests that prove both coherent selected facts and fail-closed
  missing/incoherent fact cases.
- Run `cmake --build --preset default` before accepting any code-changing
  step.
- For code-changing steps, run the delegated backend subset at minimum; use
  broader or full CTest when the supervisor asks or when multiple migrated
  paths are accumulated.

## Ordered Steps

### Step 1: Inventory Selected Storage And Initializer Recovery Sites

Goal: pin the first concrete local-memory and global/object-data migration
targets without re-running the full 418 audit.

Primary targets:
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- local memory/storage consumers referenced by the Idea 417 handoff

Actions:
- Cite the exact 413 taxonomy rows and 418 audit rows consumed by this idea.
- Identify the smallest local-memory path with recoverable layout/range facts.
- Identify the smallest global/object-data path from
  `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001` or
  `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` that can be migrated first.
- Record which portions are explicitly not owned by Idea 417 because they
  belong to Idea 415 symbol/value materialization.

Completion check:
- `todo.md` names the selected first-slice fact families, target surfaces, row
  IDs, owner classifications, and non-owned Idea 415 handoff boundaries.

### Step 2: Publish A Storage/Initializer Contract Plan

Goal: create the durable contract plan required by the source idea before
implementation.

Primary target:
- `docs/prepared_fact_contracts/storage_initializer_contract_plan.md`

Actions:
- Define selected local storage fact payloads: extent, alignment, byte range,
  overlay/alias authority, and memory provenance.
- Define selected global/object-data fact payloads: labels, section/publication
  identity, emitted bytes, zero-fill, relocations, byte ranges, and unsupported
  object-data markers.
- Map every selected payload to the consumed 413/418 row IDs.
- State which selected facts classify as `producer_missing`,
  `producer_incoherent`, `target_unsupported_but_coherent`, or
  `pre_prepared_semantic_failure`.
- Record the Idea 415 boundary for symbol/value materialization fallback.

Completion check:
- The plan artifact names the consumed taxonomy/audit rows and gives enough
  payload detail for an executor to implement verifier checks without
  re-deriving source intent.

### Step 3: Add Prepared Fact And Verifier Classification Support

Goal: make selected storage/global contract facts visible to the shared
prepared contract verifier.

Primary targets:
- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- prepared storage/global fact definitions or publication structures selected
  in Step 1

Actions:
- Add or extend fact-family enum/report support for the selected storage and
  global initializer families.
- Implement verifier functions for required local storage and global/object
  payload completeness.
- Classify missing selected facts as `producer_missing`.
- Classify contradictory selected facts as `producer_incoherent`.
- Classify complete but unsupported object-data forms as
  `target_unsupported_but_coherent`.
- Add focused prealloc/verifier tests for coherent, missing, incoherent, and
  unsupported-but-coherent selected cases.

Completion check:
- Focused verifier tests pass and reports carry the expected owner class,
  fact family, `fail_closed` state, and diagnostic detail.

### Step 4: Migrate One Local-Memory Consumer Path

Goal: move one selected local storage/memory-access consumer to consume
prepared facts instead of recovering layout or byte-range authority.

Primary targets:
- selected local-memory RV64 or AArch64 consumer from Step 1
- focused backend/MIR tests for aggregate or union slot layout

Actions:
- Replace selected target-side recovery with consumption of the prepared
  storage/memory-access facts.
- Attach prepared contract reports at the existing fail-closed diagnostic
  boundary where possible.
- Preserve coherent lowering over complete facts.
- Reject missing or contradictory selected facts before target fallback.

Completion check:
- Focused tests prove one coherent local-memory path and at least one
  fail-closed missing/incoherent local storage fact.
- `cmake --build --preset default` and the delegated backend subset pass.

### Step 5: Migrate One Global/Object-Data Consumer Path

Goal: move one selected RV64 global/object-data path away from raw BIR/global
reconstruction.

Primary targets:
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused packed/fp128/global initializer admission tests

Actions:
- Consume prepared object/global facts for labels, sections, byte ranges,
  emitted bytes, zero-fill, relocations, and unsupported data forms on the
  selected path.
- Remove selected fallback behavior that derives object data from raw
  initializer shape or raw name spelling.
- Keep symbol/value materialization fallback work scoped out unless it is
  required only as an explicit Idea 415 handoff note.
- Fail closed with producer-owned classification when selected prepared facts
  are absent or contradictory.

Completion check:
- Focused tests prove one coherent global/object-data path and at least one
  fail-closed missing/incoherent global or initializer fact.
- `cmake --build --preset default` and the delegated backend subset pass.

### Step 6: Acceptance Validation And Close Readiness

Goal: decide whether the source idea is complete or whether a follow-up
runbook is needed.

Actions:
- Confirm the storage/initializer contract plan names the consumed 413/418
  rows and the Idea 415 handoff boundary.
- Confirm migrated target consumers no longer recover selected
  layout/initializer semantics from source type text or BIR global structure.
- Review the diff for reject signals: testcase-shaped fixes, expectation
  weakening, raw layout fabrication, diagnostics-only churn, or unsupported
  downgrades.
- Run or record the supervisor-accepted broader validation command, normally
  default `ctest --test-dir build -j --output-on-failure`.
- Record close-readiness evidence in `todo.md`.

Completion check:
- `todo.md` records accepted proof, overfit review, remaining scope, and a
  recommendation to close, rewrite, or split the source idea.
