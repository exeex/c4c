Status: Active
Source Idea Path: ideas/open/417_prepared_storage_layout_and_initializer_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Selected Storage And Initializer Recovery Sites

# Current Packet

## Just Finished

Step 1 inventory for
`ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`
selected the first prepared storage/global recovery slice.

Selected fact families:
- `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`: object/global emitted
  bytes, relocations, zero-fill, symbol references, section ownership,
  byte-range facts, and unsupported object-data forms.
- `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`: prepared load/store base, offset,
  size, alignment, provenance, selected access authority, and access-plan
  shape.
- `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`: object labels, linkage,
  section/publication decisions, visibility, global initializer admission, and
  emitted-record identity.
- Storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`: decoded
  storage/home authority when local storage layout, byte range, alignment, or
  frame/object identity feeds memory lowering.

Selected target surfaces:
- First local-memory/storage path: prepared aggregate/byval or frame-slot local
  storage consumers that already depend on explicit prepared transport,
  address materialization, value-home storage, memory access, and publication
  facts. The first migration should use the smallest local path from
  `418-AUD-RV64-BYVAL-COHERENT-001` or
  `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`, with AArch64 publication rows
  as reference boundaries only.
- First global/object-data path:
  `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp` for
  `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001` and
  `src/backend/mir/riscv/codegen/object_emission.cpp` for
  `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`.

Consumed 418 audit rows:
- `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`: Idea 417 owns prepared emitted
  bytes, labels, symbol identity as object/publication identity, byte ranges,
  memory-access provenance, initializer/storage facts, and fail-closed
  classifications for missing or contradictory global facts.
- `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`: Idea 417 owns prepared object
  labels, sections, bytes, relocations, zero-fill, unsupported object-data
  forms, and publication identity before RV64 object emission.
- `418-AUD-RV64-BYVAL-COHERENT-001`: reference local storage path for complete
  aggregate transport and memory-access facts; keep coherent lowering over the
  explicit stack payload without deriving aggregate layout.
- `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`: local storage/access recovery
  boundary where missing route/storage facts should be `producer_missing` and
  contradictory address-materialization or home data should be
  `producer_incoherent`.
- `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`: publication/value-home
  reference boundary for required-vs-optional local storage facts.
- `418-AUD-A64-STACK-PRESERVE-RECOVERY-001`: publication/value-home reference
  boundary for required source home and store-shape facts.

Owner classifications for the selected slice:
- Missing selected layout, byte range, object bytes, labels, sections,
  relocations, zero-fill, publication identity, or memory-access provenance:
  `producer_missing`.
- Contradictory selected slot ranges, byte ranges, symbol identity,
  initializer bytes, relocations, section/publication facts, or access
  provenance: `producer_incoherent`.
- Complete but not yet lowerable object-data or storage/access forms:
  `target_unsupported_but_coherent`.
- Invalid pre-prepared initializer semantics:
  `pre_prepared_semantic_failure`.

Non-owned Idea 415 handoff:
- Idea 415 owns only the symbol/value materialization fallback portion of
  `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`: fallback symbol spelling, raw
  value-name normalization, and prepared materialization facts for target code
  that currently strips or recovers raw names.
- Idea 415 does not own object bytes, initializer layout, byte ranges, section
  choice, zero-fill, relocations, object labels, emitted-record identity, or
  object-data publication; those remain Idea 417 storage/initializer contract
  work.

## Suggested Next

Execute Step 2 by creating
`docs/prepared_fact_contracts/storage_initializer_contract_plan.md` with the
payload contract for the selected local storage and global/object-data facts
above.

## Watchouts

- Do not absorb Idea 415 symbol/value materialization fallback work into Idea
  417.
- Do not let RV64 recover initializer bytes, object labels, sections,
  relocations, zero-fill, or byte ranges from raw BIR/global structure once a
  selected prepared contract is required.
- Keep AArch64 rows as reference boundaries for publication/value-home
  classification unless the supervisor explicitly selects an AArch64 migration
  path.

## Proof

Docs-only inventory packet; no build or CTest required. Local sanity check:
`git diff -- todo.md`.
