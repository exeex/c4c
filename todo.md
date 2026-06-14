Status: Active
Source Idea Path: ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Edge-Publication Surfaces and Consumers

# Current Packet

## Just Finished

Completed `plan.md` Step 1, "Inventory Edge-Publication Surfaces and
Consumers", as an analysis-only blocker-map inventory.

Prepared compatibility surfaces:

- Public prepared lookup state lives in
  `src/backend/prealloc/publication_plans.hpp`: `PreparedEdgePublication`,
  `PreparedEdgePublicationLookups`, `PreparedEdgePublicationKey`,
  `PreparedEdgePublicationLookupStatus`, `PreparedEdgePublicationSourceProducer`,
  `PreparedEdgePublicationSourceProducerLookups`, and
  `find_unique_indexed_prepared_edge_publication(...)`.
- Prepared edge-publication lookup construction lives in
  `src/backend/prealloc/prepared_lookups.cpp`:
  `make_prepared_edge_publication_lookups(...)` builds
  `edge_publications.publications` from prepared join transfers, parallel-copy
  bundles, value homes, source producer facts, source memory facts, and move
  resolutions.
- Observable prepared edge-publication statuses are `Available`,
  `MissingPredecessorLabel`, `MissingSuccessorLabel`,
  `MissingDestinationValue`, and `MissingDestinationHome`.
- Prepared source-producer names are compatibility-visible:
  `unknown`, `immediate`, `load_local`, `load_global`, `cast`, `binary`, and
  `select_materialization`.
- Prepared source-memory status rows for load-local publication sources are
  compatibility-visible: `unavailable`, `available`,
  `missing_prepared_memory_access`, and
  `incomplete_prepared_memory_access`.
- Prepared block-entry publication lookup in
  `src/backend/prealloc/value_locations.hpp` /
  `src/backend/prealloc/prepared_lookups.cpp` exposes
  `PreparedCurrentBlockEntryPublication`,
  `PreparedCurrentBlockEntryPublicationQueryInputs`, and statuses
  `available`, `missing_names`, `missing_value_locations`,
  `missing_successor_label`, `missing_destination_value`,
  `missing_destination_home`, `missing_publication`, and
  `publication_unavailable`. It can attribute a prepared block-entry
  publication to Route 4 through
  `attribute_route4_block_entry_publication_if_agreeing(...)` without making
  prepared lookup status private.

Route/BIR surfaces:

- Route 4 publication availability records live in
  `src/backend/bir/bir.hpp` / `src/backend/bir/bir.cpp`:
  `Route4CurrentBlockPublicationRecord`,
  `Route4BlockEntryPublicationRecord`, `Route4PublicationValueRecord`,
  `Route4PublicationAvailabilityIndex`,
  `route4_current_block_publication_record(...)`,
  `route4_block_entry_publication_record(...)`,
  `route4_build_publication_availability_index(...)`,
  `route4_find_current_block_publication(...)`, and route-index validation
  helpers.
- Route 4 status names are `unavailable`, `available`, `missing_block`,
  `missing_value`, `missing_publication`, `alternate_source`, and `no_match`.
  Its source kinds mirror Route 1 producer kinds: immediate, load-local,
  load-global, cast, binary, select materialization, or unknown.
- Route 5 edge/source publication records live in
  `src/backend/bir/bir.hpp` / `src/backend/bir/bir.cpp`:
  `Route5CfgEdgePublicationRecord`, `Route5CurrentBlockJoinSourceRecord`,
  `Route5PublicationValueRecord`, `Route5EdgeJoinSourceIndex`,
  `route5_cfg_edge_publication_record(...)`,
  `route5_current_block_join_source_records(...)`,
  `route5_build_edge_join_source_index(...)`,
  `route5_find_cfg_edge_publication(...)`, and
  `route5_find_current_block_join_source(...)`.
- Route 5 status names are `unavailable`, `available`, `no_source`,
  `memory_source`, `missing_predecessor`, `missing_successor`,
  `missing_destination`, `missing_publication`, `missing_source_value`,
  `missing_source_producer`, `missing_source_memory_access`,
  `incomplete_source_memory_access`, and `no_match`.
- Route 5 records carry predecessor/successor labels, destination identity,
  source identity, source producer kind/instruction index, and for load-local
  sources an optional Route 3 `source_memory_access` identity. The
  `memory_source` row is semantic evidence, but the precise fallback status
  spelling remains compatibility-owned.
- MIR-facing helper accessors live in `src/backend/mir/query.hpp` /
  `src/backend/mir/query.cpp`: `BirCfgEdgePublicationSourceIdentity`,
  `find_bir_cfg_edge_publication_source_identity(...)`,
  `BirCurrentBlockJoinSourceIdentity`, and
  `find_bir_current_block_join_source_identity(...)`. These translate Route 5
  statuses to MIR status rows and expose same-block producer/source identities.

x86 consumers and gaps:

- x86 prepared edge-publication move consumption exists in
  `src/backend/mir/x86/prepared/prepared.hpp` /
  `src/backend/mir/x86/prepared/dispatch.cpp` through
  `EdgePublicationMoveIntent`, `consume_edge_publication_move_intent(...)`, and
  `append_edge_publication_move_instruction(...)`.
- x86 prepared statuses are `Available`, `MissingSharedLookups`,
  `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`, and
  `UnsupportedDestinationHome`; these are observable dispatch/status rows and
  must remain compatibility-owned.
- x86 exact output is target policy: source operand spelling such as
  `DWORD PTR [rsp + N]`, register/immediate operand spelling, and final
  instruction text `mov <dst>, <src>` are not Route 4/5 semantic identity.
- x86 module output consumes those prepared intents in
  `src/backend/mir/x86/module/module.cpp`
  `append_prepared_compare_join_parallel_copy(...)`, with error text such as
  "compare-join edge has no shared prepared edge-publication lookup authority"
  and "compare-join edge parallel-copy step has no shared prepared
  edge-publication fact" remaining compatibility-owned.
- Observed gap for later Step 3: x86 edge-publication consumption does not
  carry Route 4/5 agreement fields like `route5_edge_status`,
  `route5_edge_source_agrees`, or a Route 5 cfg-edge record input. The x86
  status/module-output rows consume prepared lookup authority directly.
- Existing x86 publication tests
  `tests/backend/mir/backend_x86_publication_plan_reuse_test.cpp` and
  `tests/backend/mir/backend_x86_store_source_publication_plan_reuse_test.cpp`
  cover scalar/store publication plans, not a Route 4/5 edge-publication
  agreement consumer.

riscv consumers:

- riscv prepared edge-publication move handling lives in
  `src/backend/mir/riscv/codegen/emit.hpp` /
  `src/backend/mir/riscv/codegen/emit.cpp` through
  `EdgePublicationMoveIntent`, `consume_edge_publication_move_intent(...)`,
  and `append_edge_publication_move_instruction(...)`.
- riscv status rows mirror the prepared move intent statuses:
  `Available`, `MissingSharedLookups`, `MissingPublication`,
  `UnsupportedPublication`, `UnsupportedSourceHome`, and
  `UnsupportedDestinationHome`.
- riscv has explicit Route 5 agreement attachment:
  `RiscvEdgePublicationMoveAdapter::attach_route5_edge_agreement(...)`,
  `route5_edge_source_agrees_with_prepared_publication(...)`, and
  `route3_source_memory_agrees_with_prepared_publication(...)` populate
  `route5_edge_status`, `route5_edge_source_agrees`, and
  `route3_source_memory_agrees`.
- riscv exact output stays target-owned: `mv`, `li`, `lw`, stack offsets,
  register names, destination register bank/placement, stack-slot offsets,
  pointer base register, immediate legality, and `instruction_text` are not the
  Route 4/5 semantic fact.
- riscv fallback and unsupported publication rows remain compatibility-owned:
  missing shared lookups/publication, unsupported publication, unsupported
  source home, unsupported destination home, source-memory fallback rows, and
  the load-local memory source checks around base kind, offset, size, align,
  volatility, and address space.

Compatibility-owned surfaces to preserve:

- Prepared helper/oracle names, lookup keys, status strings, fallback
  publication rows, source producer names, source-memory statuses, x86
  dispatch/status/module-output diagnostics, riscv status rows, exact
  instruction text, register selection, carrier/helper choice, storage/layout
  policy, and target-specific operand/addressing spelling.

## Suggested Next

Execute `plan.md` Step 2, "Select One Candidate Edge-Publication Identity
Fact", as an analysis-only packet.

## Watchouts

- Keep source idea 251 unchanged unless durable intent truly changes.
- Do not implement an adapter during the blocker-map inventory.
- Preserve prepared edge-publication lookup/status, helper/oracle names,
  fallback publication rows, x86 dispatch/status/module output, riscv status
  fields, and riscv/x86 instruction/output strings as compatibility-owned until
  proven otherwise.
- Step 2 should choose one narrow Route 4/5 identity; do not fold in broader
  publication, scalar/store publication plans, or target output policy.
- Treat testcase-shaped shortcuts, expectation weakening, helper renames, and
  output rewrites as route drift.

## Proof

No build or test proof required; analysis-only packet. Local validation:
`git diff --check -- todo.md`.
