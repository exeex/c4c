# Prepared Storage Layout And Initializer Contract Plan

Status: Published for idea 417 Step 2
Source Idea: `ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`

This document defines the first durable prepared contract slice for local
storage layout, memory-access authority, global/object data, and initializer
facts. The selected targets are the RV64 global/object recovery rows from idea
418 plus the smallest local storage/reference rows needed to keep storage
payloads tied to explicit prepared facts.

## Consumed Rows

Taxonomy rows consumed from
`docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`:

- `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`
- `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`
- `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`
- storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`

Audit rows consumed from
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md`:

- `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`
- `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`
- `418-AUD-RV64-BYVAL-COHERENT-001`
- `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`
- `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`
- `418-AUD-A64-STACK-PRESERVE-RECOVERY-001`

## Selected Local Storage Payloads

| Payload | Required prepared fact authority | Consumed taxonomy rows | Consumed audit rows | Initial owner classification |
| --- | --- | --- | --- | --- |
| Storage object extent | The producer publishes the selected storage object's byte extent before target lowering. The extent covers the complete slot/object selected for the local path, not a target-guessed aggregate width. | `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | `418-AUD-RV64-BYVAL-COHERENT-001`, `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001` | Missing extent is `producer_missing`; conflicting extents for the same selected object are `producer_incoherent`; a complete extent for a storage form the target cannot lower is `target_unsupported_but_coherent`. |
| Storage alignment | The producer publishes required alignment for the selected slot/object and for any access plan that constrains target load/store selection. | `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | `418-AUD-RV64-BYVAL-COHERENT-001`, `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`, `418-AUD-A64-STACK-PRESERVE-RECOVERY-001` | Missing required alignment is `producer_missing`; contradictory slot/access alignment is `producer_incoherent`; complete but unsupported alignment-sensitive access forms are `target_unsupported_but_coherent`. |
| Byte range | The producer publishes the base object identity, offset, size, and end range consumed by the selected memory access or aggregate transport. Targets may choose an addressing mode over the range but must not invent the range. | `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | `418-AUD-RV64-BYVAL-COHERENT-001`, `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`, `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001` | Missing base/range/size is `producer_missing`; contradictory object identity, offset, size, or range coverage is `producer_incoherent`; complete unsupported addressing forms are `target_unsupported_but_coherent`. |
| Overlay and alias authority | The producer states whether the selected storage object may overlay another slot/object and which aliasing relationship authorizes that overlap. Targets must not widen all slots or reinterpret aggregate/union layout to make accesses fit. | `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-BYVAL-COHERENT-001`, `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`, `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`, `418-AUD-A64-STACK-PRESERVE-RECOVERY-001` | Missing required alias/overlay authority is `producer_missing`; overlapping ranges without matching authority are `producer_incoherent`; coherent overlay forms without selected target support are `target_unsupported_but_coherent`. |
| Memory provenance | The producer publishes the source storage/home, publication route, access kind, and selected load/store authority that prove the access belongs to the prepared object. | `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, storage-layout portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`, `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`, `418-AUD-A64-STACK-PRESERVE-RECOVERY-001` | Missing provenance for a required access is `producer_missing`; contradictions between home, publication, route, and access authority are `producer_incoherent`; complete access plans outside the selected lowering subset are `target_unsupported_but_coherent`. |

## Selected Global And Object-Data Payloads

| Payload | Required prepared fact authority | Consumed taxonomy rows | Consumed audit rows | Initial owner classification |
| --- | --- | --- | --- | --- |
| Object labels | The producer publishes the stable object label or labels used by target assembly and object emission. Labels are object/publication identity, not raw name fallbacks. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`, `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` | Missing selected labels are `producer_missing`; conflicting labels, linkage names, or emitted-record identity are `producer_incoherent`. |
| Section and publication identity | The producer publishes section choice, visibility/linkage, object publication identity, and emitted-record identity before RV64 emission. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`, `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` | Missing publication or section facts are `producer_missing`; stale or contradictory section/publication facts are `producer_incoherent`; coherent sections not yet supported by the selected target route are `target_unsupported_but_coherent`. |
| Emitted bytes | The producer publishes the exact byte payload for selected initialized object data. RV64 object emission consumes bytes and must not reinterpret raw BIR initializer values or element lists. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001` | `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`, `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` | Missing required bytes are `producer_missing`; byte payloads that conflict with range, size, relocation, or zero-fill facts are `producer_incoherent`; invalid pre-prepared initializer semantics are `pre_prepared_semantic_failure`. |
| Zero-fill | The producer publishes explicit zero-fill ranges or whole-object zero-fill status for selected uninitialized or partially initialized objects. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`, `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001` | Missing selected zero-fill facts are `producer_missing`; overlap or size conflicts between zero-fill and emitted bytes are `producer_incoherent`; complete zero-fill forms unsupported by the selected emitter are `target_unsupported_but_coherent`. |
| Relocations and symbol references | The producer publishes relocations, referenced symbols, addends, byte offsets, and width/kind data for object records that require symbolic data. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`, `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` | Missing selected relocation facts are `producer_missing`; relocation/range/symbol contradictions are `producer_incoherent`; complete relocation forms outside the selected target route are `target_unsupported_but_coherent`. |
| Object byte ranges | The producer publishes each object data record's byte range, including initialized byte spans, zero-fill spans, relocation spans, and whole-object extent. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`, `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` | Missing range facts are `producer_missing`; overlaps, gaps in required complete records, or size mismatches are `producer_incoherent`; coherent range layouts unsupported by the target emitter are `target_unsupported_but_coherent`. |
| Unsupported object-data markers | The producer publishes an explicit unsupported-but-coherent marker when initializer/object data is complete and semantically valid but outside the selected emission subset. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`, `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` | Missing marker for a required but unlowered object-data form is `producer_missing`; marker/data contradictions are `producer_incoherent`; complete marked forms are `target_unsupported_but_coherent`; invalid initializer semantics remain `pre_prepared_semantic_failure`. |

## Classification Rules

- `producer_missing`: a selected required prepared payload is absent before the
  target consumer would otherwise reconstruct it. This includes missing local
  extents, alignment, byte ranges, alias/overlay authority, memory provenance,
  object labels, section/publication identity, emitted bytes, zero-fill,
  relocations, object byte ranges, and required unsupported-form markers.
- `producer_incoherent`: a selected prepared payload exists but contradicts
  another selected fact or lacks required payload for its shape. Examples are
  overlapping byte ranges without alias authority, mismatched object extent and
  emitted bytes, conflicting label/publication identity, relocation spans
  outside the object range, and contradictory memory provenance.
- `target_unsupported_but_coherent`: selected prepared facts are complete and
  internally consistent, but the selected RV64/AArch64 consumer does not yet
  lower that coherent form. This applies only after verifier checks prove the
  facts are complete; it must not hide missing or contradictory producer data.
- `pre_prepared_semantic_failure`: initializer or storage semantics are invalid
  before prepared publication. The verifier/reporting path should preserve this
  classification instead of converting invalid source/BIR semantics into target
  unsupported status.

## Initial Implementation Boundary

The first local-memory implementation path should use the smallest selected
local storage consumer from `418-AUD-RV64-BYVAL-COHERENT-001` or
`418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`. AArch64 rows
`418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001` and
`418-AUD-A64-STACK-PRESERVE-RECOVERY-001` are reference boundaries for
required-vs-optional publication/value-home classification unless a later
supervisor packet selects an AArch64 migration.

The first global/object-data implementation path should target
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp` for
`418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001` and
`src/backend/mir/riscv/codegen/object_emission.cpp` for
`418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`.

## Idea 415 Boundary

Idea 415 owns only the symbol/value materialization fallback portion of
`418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`: fallback symbol spelling, raw
value-name normalization, and prepared materialization facts for target code
that currently strips or recovers raw names.

Idea 417 owns object/global bytes, initializer layout, byte ranges, zero-fill,
sections, relocations, object labels, emitted-record identity, publication
identity, memory-access provenance, and unsupported object-data markers. RV64
must not use Idea 415 materialization fallback work as a substitute for these
storage/initializer facts.

## Verifier Handoff

Step 3 should add prepared verifier support that can answer these questions for
the selected slice before target consumers run:

- Does every selected local storage access have extent, alignment, byte range,
  overlay/alias authority, and memory provenance?
- Does every selected object/global record have labels, section/publication
  identity, emitted bytes or zero-fill facts, relocation facts where required,
  object byte ranges, and explicit unsupported markers where applicable?
- Are missing facts reported as `producer_missing` and contradictory facts
  reported as `producer_incoherent`?
- Are complete but not-yet-lowerable records reported as
  `target_unsupported_but_coherent` without weakening the prepared contract?
- Are invalid initializer semantics preserved as
  `pre_prepared_semantic_failure` rather than recovered in target emission?
