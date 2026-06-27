Status: Active
Source Idea Path: ideas/open/417_prepared_storage_layout_and_initializer_contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish A Storage/Initializer Contract Plan

# Current Packet

## Just Finished

Step 2 published
`docs/prepared_fact_contracts/storage_initializer_contract_plan.md` for
`ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`.

The plan defines selected local storage payloads for extent, alignment, byte
range, overlay/alias authority, and memory provenance. It defines selected
global/object-data payloads for object labels, section/publication identity,
emitted bytes, zero-fill, relocations, object byte ranges, and unsupported
object-data markers.

Every selected payload maps to the consumed taxonomy rows
`TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`,
`TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`,
`TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, and storage-layout portions of
`TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`, plus the consumed 418 rows
`418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`,
`418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001`,
`418-AUD-RV64-BYVAL-COHERENT-001`,
`418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`,
`418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`, and
`418-AUD-A64-STACK-PRESERVE-RECOVERY-001`.

The plan records the selected owner classifications:
`producer_missing`, `producer_incoherent`,
`target_unsupported_but_coherent`, and `pre_prepared_semantic_failure`.
It also records the Idea 415 boundary: symbol/value materialization fallback
stays with Idea 415, while object bytes, initializer layout, byte ranges,
sections, relocations, zero-fill, object labels, publication identity, memory
provenance, and unsupported object-data markers remain Idea 417 work.

## Suggested Next

Execute Step 3 by adding prepared fact/verifier classification support for the
selected storage/global payloads defined in
`docs/prepared_fact_contracts/storage_initializer_contract_plan.md`.

## Watchouts

- Step 3 should preserve the Idea 415 boundary: raw symbol spelling and
  value-name normalization are not object-data contract payloads.
- Missing selected storage/object facts should classify as `producer_missing`;
  contradictory selected facts should classify as `producer_incoherent`.
- Complete but not-yet-lowerable object-data or storage/access forms should be
  `target_unsupported_but_coherent`; invalid pre-prepared initializer semantics
  should remain `pre_prepared_semantic_failure`.

## Proof

Docs-only contract-plan packet; no build or CTest required. Local sanity
check: `git diff -- docs/prepared_fact_contracts/storage_initializer_contract_plan.md todo.md`.
