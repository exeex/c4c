Status: Active
Source Idea Path: ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Source-Producer Authority

# Current Packet

## Just Finished

Completed Step 1, `Inventory Source-Producer Authority`, as an analysis-only
inventory for idea 267.

Prepared authority:

- `PreparedFunctionLookups` publicly stores
  `edge_publication_source_producers` as a
  `PreparedEdgePublicationSourceProducerLookups` field at
  `src/backend/prealloc/prepared_lookups.hpp:15`.
- The prepared source-producer row type is
  `PreparedEdgePublicationSourceProducer`, with kind, prepared block label,
  instruction index, and typed payload pointers for `LoadLocal`, `LoadGlobal`,
  `Cast`, `Binary`, and `SelectMaterialization` at
  `src/backend/prealloc/publication_plans.hpp:236`.
- The lookup container is
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name`,
  keyed by `ValueNameId`, at `src/backend/prealloc/publication_plans.hpp:248`.
- `make_prepared_edge_publication_source_producer_lookups(...)` constructs the
  lookup from the prepared BIR function by walking all blocks/instructions and
  publishing source producers for `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`,
  and `SelectMaterialization` results at
  `src/backend/prealloc/select_chain_lookups.cpp:172`.
- `publish_source_producer(...)` records the first producer for a value and
  overwrites duplicate value-name producers with a default/unknown producer,
  preserving duplicate conflict as fail-closed unknown authority at
  `src/backend/prealloc/select_chain_lookups.cpp:38`.
- `find_indexed_prepared_edge_publication_source_producer(...)` exposes lookup
  reads by value name at `src/backend/prealloc/select_chain_lookups.cpp:276`.
- `make_prepared_function_lookups(...)` constructs the source-producer lookup
  and stores it into `PreparedFunctionLookups::edge_publication_source_producers`
  at `src/backend/prealloc/prepared_lookups.cpp:1517`.
- `make_prepared_edge_publication_lookups(...)` separately rebuilds local
  source-producer lookups, copies matching producer facts into
  `PreparedEdgePublication` rows via `apply_source_producer_fact(...)`, and
  augments `LoadLocal` rows with prepared memory-access evidence through
  `apply_source_memory_access_fact(...)` at
  `src/backend/prealloc/prepared_lookups.cpp:1336` and
  `src/backend/prealloc/prepared_lookups.cpp:401`.

Route 5 / BIR authority:

- BIR Route 5 record schema carries source value identity, source-producer kind,
  source-producer instruction pointer/index, block label id for edge records,
  and optional source-memory identity for `LoadLocal` edge publications in
  `Route5CfgEdgePublicationRecord` and `Route5CurrentBlockJoinSourceRecord` at
  `src/backend/bir/bir.hpp:1657`.
- `route5_publication_source_kind(...)` maps Route 1 same-block producer kinds
  into Route 5 source-producer kinds at `src/backend/bir/bir.cpp:1698`.
- `route5_cfg_edge_publication_record(...)` derives edge source-producer
  identity from the predecessor block Route 1 producer index, records
  `MissingSourceProducer`, `MemorySource`, `Available`, `NoSource`, and
  `MissingPublication` statuses, and attaches Route 3 memory identity for
  `LoadLocal` sources at `src/backend/bir/bir.cpp:1719`.
- `route5_current_block_join_source_records(...)` derives current-block join
  source-producer identity from the successor block Route 1 producer index and
  publishes per-incoming records at `src/backend/bir/bir.cpp:1849`.
- `route5_build_edge_join_source_index(...)` publishes Route 5 edge records,
  join records, and value records for a BIR function at
  `src/backend/bir/bir.cpp:2038`.
- MIR query conversion preserves Route 5 source-producer identity in
  `route5_edge_source_producer_to_mir(...)`,
  `route5_join_source_producer_to_mir(...)`, and
  `route5_edge_record_to_mir(...)` at `src/backend/mir/query.cpp:621`.
- `find_bir_current_block_join_source_identity(...)` can read indexed or
  rebuilt Route 5 join records and publishes MIR-facing join source facts at
  `src/backend/mir/query.cpp:1704`.

Retained compatibility and exposure surfaces:

- Public helper APIs in `src/backend/prealloc/select_chain_lookups.hpp:112` and
  `src/backend/prealloc/select_chain_lookups.hpp:117` continue to construct and
  expose prepared source-producer lookups directly.
- Prepared same-block/materialization helpers read the prepared lookup through
  `find_prepared_current_block_publication_consumption(...)`,
  `find_prepared_same_block_scalar_producer(...)`, integer-constant evaluation,
  and load-local/global helpers at `src/backend/prealloc/prepared_lookups.cpp:2076`.
- Prepared select-chain and store-source printer rows emit
  `source_producer`, `source_producer_block`, and `source_producer_inst`
  fields at `src/backend/prealloc/prepared_printer/select_chains.cpp:76`.
- AArch64 still threads `PreparedFunctionLookups` in lowering context at
  `src/backend/mir/aarch64/module/module.hpp:73` and constructs it per function
  at `src/backend/mir/aarch64/codegen/traversal.cpp:71`.
- x86 `ConsumedPlans` stores an optional `PreparedFunctionLookups` aggregate and
  exposes it through `shared_function_lookups()` at
  `src/backend/mir/x86/x86.hpp:14`; `consume_plans(...)` fills it through
  `consume_prepared_function_lookups(...)` at `src/backend/mir/x86/x86.hpp:162`.
- x86 prepared dispatch consumes prepared edge-publication rows through
  `consume_edge_publication_move_intent(...)`, which reads
  `lookups->edge_publications` from shared prepared lookups at
  `src/backend/mir/x86/prepared/dispatch.cpp:55`.
- x86 currently has a Route 5 agreement/compatibility boundary around
  edge-publication moves:
  `route5_edge_source_agrees_with_prepared_publication(...)`,
  `agreed_route5_edge_publication_source(...)`, and
  `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`
  compare prepared publication source-producer kind/block/index and, for
  `LoadLocal`, Route 3 source-memory evidence, then allow compatibility fallback
  for non-`LoadLocal` source-producer kinds at
  `src/backend/mir/x86/module/module.cpp:4110`.

## Suggested Next

Execute Step 2 by deciding whether the x86 edge-publication move
agreement/compatibility gate is the real consumer boundary for the
`edge_publication_source_producers` relation, and map where prepared and Route 5
identity can disagree or fall back.

## Watchouts

- This inventory found an x86 Route 5 agreement check, but Step 2 still must
  decide whether it is sufficient as the concrete x86 consumer boundary for all
  required source-producer rows.
- `edge_publication_move_allowed_by_route5_agreement_or_compatibility(...)`
  deliberately allows compatibility fallback for non-`LoadLocal` producers when
  Route 5 agreement fails; Step 2/3 must classify that as compatibility
  behavior, not proven semantic replacement.
- Prepared edge-publication rows copy source-producer facts into
  `PreparedEdgePublication`; a consumer may read the copied publication fields
  without reading `edge_publication_source_producers` directly.
- No implementation files, expectations, route-debug output, printer output,
  wrappers, or baselines were changed.

## Proof

Analysis-only proof delegated by supervisor:

- `sed -n '1,260p' todo.md` after the update
- `git status --short`
- `git diff --check`

No build or CTest was required for this todo-only inventory packet. Per the
delegated proof command, no `test_after.log` update was made.
