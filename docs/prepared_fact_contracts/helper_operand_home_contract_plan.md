# Prepared Helper Operand Home Contract Plan

Source idea: `ideas/open/416_prepared_helper_operand_home_contracts.md`

## Contract Route

The first migrated helper family is the variadic entry/helper family:
`va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`.

The route replaces the current optional-bag interpretation of
`PreparedVariadicEntryHelperOperandHomes` with typed per-helper contracts.
Target helper lowering may choose an addressing mode or move sequence over a
complete prepared helper fact, but it must not infer `va_list` addresses,
result homes, aggregate payload homes, copy homes, or access plans when the
prepared fact is absent or incoherent.

## Consumed Taxonomy Rows

- `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001`: owns variadic entry helper
  operand homes and access plans. Missing entry/helper facts are
  `producer_missing`; present but incomplete or wrong-helper records are
  `producer_incoherent`.
- `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`: owns typed helper and
  intrinsic operand contracts beyond the initial variadic surface. Missing
  helper operand homes are `producer_missing`; wrong helper kind or incomplete
  per-helper payloads are `producer_incoherent`; complete helper forms without
  a target lowering route are `target_unsupported_but_coherent`.

## Consumed Audit Rows

- `418-AUD-RV64-INLINEASM-COHERENT-001`: RV64 inline-asm lowering is a
  coherent reference for complete carriers and operand homes. Missing operand
  homes, tied-home contradictions, or address selections remain
  producer-owned helper facts, not target reconstruction.
- `418-AUD-A64-VARIADIC-COHERENT-001`: AArch64 variadic helper lowering is a
  coherent reference for complete entry resources, helper operand homes, and
  access plans. Incomplete variadic facts keep the idea 413
  missing/incoherent owner surface and must not be repaired in target lowering.
- `418-AUD-A64-INLINEASM-COHERENT-001`: AArch64 inline-asm lowering is a
  coherent reference for complete carriers with explicit operand homes,
  immediates, tied homes, and address selections. Missing carrier fields remain
  producer/helper contract issues; complete but unlowerable forms may be
  target unsupported.

Idea 418 records the explicit idea 416 no-applicable-recovery-row note: the
audit found no selected recovery row requiring immediate idea 416 cleanup.
Therefore this plan uses the coherent helper rows as boundary examples and
adds fail-closed owner decisions for any missing or incoherent variadic helper
carrier fields discovered during implementation.

## Helper Owner Decisions

| Helper | Required typed payload | Missing owner decision | Incoherent owner decision |
| --- | --- | --- | --- |
| `va_start` | `destination_va_list`, `destination_va_list_address`, complete variadic entry resources, `va_list` layout, register-save area, overflow area, and helper scratch resources. | Missing selected `va_start` helper facts or any required destination/address/entry-resource field is `producer_missing`. | A present non-`va_start` helper record, partial destination/address payload, contradictory entry-resource payload, or complete payload with the wrong helper kind is `producer_incoherent`. |
| scalar `va_arg` | `source_va_list`, scalar result home, scalar value type/size/alignment, source/progression/overflow field selections, slot size, and stride fields in the scalar access plan. | Missing selected scalar `va_arg` helper facts, source `va_list`, result home, scalar ABI payload, or scalar access-plan field is `producer_missing`. | A present non-scalar-`va_arg` helper record, incomplete scalar access plan, wrong-helper payload mix, contradictory source/progression fields, or invalid scalar ABI payload is `producer_incoherent`. |
| aggregate `va_arg` | `source_va_list`, aggregate destination payload home, aggregate payload size/alignment, source field, payload offset, source slot size, copy size/alignment, progression field/stride, and any register-save lane payload needed by the selected aggregate source class. | Missing selected aggregate `va_arg` helper facts, source `va_list`, aggregate destination payload, aggregate ABI payload, or aggregate access-plan field is `producer_missing`. | A present non-aggregate-`va_arg` helper record, incomplete aggregate access plan, wrong-helper payload mix, contradictory payload/write-address data, invalid copy dimensions, or register-save lane data inconsistent with the selected source class is `producer_incoherent`. |
| `va_copy` | `destination_va_list` and `source_va_list` homes for the copy helper. | Missing selected `va_copy` helper facts, destination `va_list`, or source `va_list` is `producer_missing`. | A present non-`va_copy` helper record, partial copy payload, or helper payload mixed with scalar/aggregate access-plan fields is `producer_incoherent`. |

## Consumer Boundary

Migrated target consumers must verify the typed helper payload before lowering
and must fail closed on `producer_missing` or `producer_incoherent` reports.
They may report `target_unsupported_but_coherent` only after the typed payload
is complete and internally coherent but the selected target route cannot lower
that complete helper form.
