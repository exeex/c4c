Status: Active
Source Idea Path: ideas/open/162_bir_publication_availability_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Publication Availability Surfaces

# Current Packet

## Just Finished

Completed Step 1 for
`ideas/open/162_bir_publication_availability_annotation_schema.md`.
Inventoried the Route 4 publication availability surfaces and recorded the
concrete Step 2 schema targets.

Prepared current-block publication read surfaces:

- `prepare::find_prepared_current_block_publication_consumption(...)` in
  `src/backend/prealloc/select_chain_lookups.hpp/.cpp`.
- Oracle record:
  `prepare::PreparedCurrentBlockPublicationConsumption`.
- Oracle payload to compare: `available`, `source_producer`, `instruction`,
  `produced_value`, `instruction_index`, `value_name`, and
  `source_producer_kind`.
- Producer lookup carrier:
  `prepare::PreparedEdgePublicationSourceProducerLookups` keyed by
  `ValueNameId`.
- Producer payload:
  `prepare::PreparedEdgePublicationSourceProducer` with
  `kind`, `block_label`, `instruction_index`, and exactly one of
  `load_local`, `load_global`, `cast`, `binary`, or `select`.

Prepared block-entry publication read surfaces:

- `prepare::find_prepared_current_block_entry_publication(...)` in
  `src/backend/prealloc/value_locations.hpp`.
- Oracle record:
  `prepare::PreparedCurrentBlockEntryPublication`.
- Oracle status:
  `PreparedCurrentBlockEntryPublicationStatus::{Available, MissingNames,
  MissingValueLocations, MissingSuccessorLabel, MissingDestinationValue,
  MissingDestinationHome, MissingPublication, PublicationUnavailable}`.
- Oracle payload to compare where semantic:
  `destination_value_id`, `destination_value_name`, and availability category.
- Additional block-entry edge-source oracle surface for source/value identity:
  `prepare::prepare_block_entry_parallel_copy_edge_source_facts(...)`,
  `prepare::prepare_edge_copy_source_facts(...)`, and
  `prepare::find_unique_indexed_block_entry_parallel_copy_edge_publication(...)`
  in `src/backend/prealloc/publication_plans.hpp/.cpp`.
- Edge-source status categories to preserve as unavailable/no-match oracles:
  `MissingPreparedLookups`, `MissingPredecessorLabel`,
  `MissingSuccessorLabel`, `MissingDestinationValue`, `MissingPublication`,
  `AmbiguousPublication`, `PublicationUnavailable`, `EdgeMismatch`,
  `UnsupportedMove`, `MoveEdgeMismatch`, `PublicationMoveMismatch`,
  `MissingSourceValue`, `MissingSourceHome`, `MissingSourceProducer`,
  `MissingSourceMemoryAccess`, and `IncompleteSourceMemoryAccess`.

BIR/MIR query surfaces to compare in Step 2:

- `mir::BirCurrentBlockPublicationIdentityRequest` and
  `mir::find_bir_current_block_publication_identity(...)`.
- `mir::BirCurrentBlockPublicationIdentity` fields:
  `available`, `source_producer`, `instruction`, `produced_value`,
  `produced_value_identity`, `produced_value_name`, `produced_value_type`,
  `instruction_index`, `value_name`, and `source_producer_kind`.
- `mir::BirBlockEntryPublicationIdentityRequest` and
  `mir::find_bir_block_entry_publication_identity(...)`.
- `mir::BirBlockEntryPublicationIdentity` fields:
  `available`, `status`, `instruction`, `phi`, `instruction_index`,
  `destination_value`, `destination_value_id`, `destination_value_identity`,
  `destination_value_name`, `destination_value_name_id`,
  `destination_value_type`, `successor_label`, and `successor_label_id`.
- Supporting BIR annotation dependencies already available:
  Route 1 producer records for source-producer identity and value identity,
  Route 2 select-chain records for select roots where relevant, and Route 3
  memory-source records only as semantic references, not publication schema.

Concrete Route 4 BIR schema targets for Step 2:

- Add `Route4PublicationAvailabilityStatus` with at least:
  `Unavailable`, `Available`, `MissingBlock`, `MissingValue`,
  `MissingPublication`, `AlternateSource`, and `NoMatch`.
- Add `Route4PublicationScope` with `CurrentBlock` and `BlockEntry`.
- Add `Route4PublicationSourceKind` mirroring the target-neutral producer
  vocabulary only: `Unknown`, `Immediate`, `LoadLocal`, `LoadGlobal`, `Cast`,
  `Binary`, and `SelectMaterialization`.
- Add a current-block block annotation record, tentatively
  `Route4CurrentBlockPublicationRecord`, with:
  `available`, `status`, `block`, `block_label`, `block_label_id`,
  `value`/`value_name`/`value_name_id`/`value_type`,
  `before_instruction_index`, `source_producer_kind`,
  `source_producer_instruction`, `source_producer_instruction_index`,
  `source_producer_block_label_id`, and `produced_value` as a
  `Route1SourceValueIdentity`.
- Add a block-entry block annotation record, tentatively
  `Route4BlockEntryPublicationRecord`, with:
  `available`, `status`, `successor_block`, `successor_label`,
  `successor_label_id`, `destination_instruction`, `phi`,
  `destination_instruction_index`, `destination_value`,
  `destination_value_name`, `destination_value_name_id`,
  `destination_value_type`, and optional source-value identity when the
  source is known from a PHI incoming or edge-source oracle.
- Add value-link records, tentatively
  `Route4PublicationValueRecord`, for produced/current-block value identity
  and consumed/block-entry destination value identity. Fields:
  `scope`, `status`, `value_role` (`Produced`, `Consumed`, `Source`),
  `value` as `Route1SourceValueIdentity`, block label/id, instruction index
  where present, and a copy/reference to the owning publication record.
- Keep unavailable answers explicit via `status`; do not make absence mean
  unavailable when the BIR block/value identity was found but publication
  availability was rejected.

Optional Step 3 lookup/index shape to keep in mind:

- `Route4PublicationAvailabilityIndex` rebuilt from Route 4 block/value
  records, grouped by function/block.
- Current-block lookup key:
  block pointer or label/id, value name/id/type, and
  `before_instruction_index`.
- Block-entry lookup key:
  successor block pointer or label/id plus destination value name/id/type.
- Optional source lookup key:
  successor block, predecessor/source label when known, destination value, and
  source value identity. This should point at Route 4 records, not duplicate
  prepared edge-publication plans.

Step 2 positive oracle cases:

- Current-block available binary producer (`%sum`) matches prepared
  `PreparedCurrentBlockPublicationConsumption`.
- Current-block producer kinds remain covered for `Binary`, `Cast`,
  `LoadLocal`, `LoadGlobal`, and `SelectMaterialization` via the existing
  `backend_prepared_lookup_helper` fixture.
- Block-entry available register-backed PHI destination matches prepared
  `PreparedCurrentBlockEntryPublicationStatus::Available`.
- Block-entry PHI identity remains available in BIR even when prepared storage
  readiness is `PublicationUnavailable`, proving Route 4 does not import
  destination storage policy.

Step 2 negative/unavailable/alternate-source/no-match cases:

- Current-block missing value, wrong block label, before-boundary/future
  producer, mismatched root type, and mismatched prepared producer fact.
- Current-block alternate-source case: prepared facts can be mismatched while
  BIR annotation remains block-derived from the actual BIR producer.
- Block-entry no PHI for prepared-ready destination:
  prepared may be available, but BIR reports `MissingPublication`.
- Block-entry prepared missing publication:
  BIR preserves destination identity/status without fabricating availability.
- Block-entry wrong successor label and wrong destination value fail closed.
- Block-entry edge-source no-match categories from
  `prepare_edge_copy_source_facts(...)`: missing prepared lookups, missing
  publication, unavailable publication, ambiguous publication,
  missing source home, mismatched move edge labels, and publication/move
  mismatch.
- Non-schema negative cases: no hook kind, destination home, storage encoding,
  stack-source extension policy, register view, immediate payload spelling,
  emitted storage availability, scalar publication emission order, whole
  `PreparedCurrentBlockPublicationConsumption`, whole
  `PreparedCurrentBlockEntryPublication`, or whole
  `PreparedEdgePublication` copied into BIR.

## Suggested Next

Execute Step 2 by adding typed Route 4 BIR publication availability annotation
records and construction helpers in `src/backend/bir/bir.hpp` and
`src/backend/bir/bir.cpp`, then extend
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` to compare the new
BIR records against the prepared and existing MIR query oracles.

## Watchouts

- Keep Route 4 annotations block/value scoped and target-neutral.
- Reuse Route 1 producer/value identity records where possible; do not create
  another producer schema.
- Do not import publication hooks, destination homes, storage encodings,
  stack-source extension policy, register views, immediate payload spelling,
  emitted storage availability, scalar publication emission policy, prepared
  move records, move-bundle order, or whole prepared publication records.
- Keep block-entry PHI identity separate from prepared destination-storage
  readiness. Prepared storage policy remains an oracle/boundary, not schema.
- Step 2 should add records/helpers only; do not switch production MIR or
  target consumers.

## Proof

Step 1 proof:

```bash
git diff --check
```

Step 2 narrow proof command to delegate:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```
