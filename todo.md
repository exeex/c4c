Status: Active
Source Idea Path: ideas/open/137_select_chain_public_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map current ownership and consumers

# Current Packet

## Just Finished

Completed plan Step 1 analysis-only mapping for the select-chain lookup surface.

Declaration ownership found:
- `PreparedSelectChainDependencyQuery`: `src/backend/prealloc/prepared_lookups.hpp:386`.
- `find_prepared_direct_global_select_chain_dependency`: duplicated public declarations at `src/backend/prealloc/prepared_lookups.hpp:1020` and `src/backend/prealloc/prepared_lookups.hpp:1029`, plus `src/backend/prealloc/publication_plans.hpp:347` and `src/backend/prealloc/publication_plans.hpp:356`.
- `find_prepared_select_chain_source_producer`: `src/backend/prealloc/publication_plans.hpp:361`.
- `find_prepared_store_source_direct_global_select_chain_dependency`: `src/backend/prealloc/publication_plans.hpp:370`.
- `find_prepared_scalar_select_chain_materialization`: `src/backend/prealloc/publication_plans.hpp:379` and `src/backend/prealloc/publication_plans.hpp:388`.

Definition ownership found:
- `PreparedSelectChainDependencyQuery` has no out-of-line definition; it is the struct at `src/backend/prealloc/prepared_lookups.hpp:386`.
- `find_prepared_select_chain_source_producer`: `src/backend/prealloc/select_chain_lookups.cpp:330`.
- `find_prepared_direct_global_select_chain_dependency`: primary overload at `src/backend/prealloc/select_chain_lookups.cpp:364`, query overload at `src/backend/prealloc/select_chain_lookups.cpp:396`.
- `find_prepared_store_source_direct_global_select_chain_dependency`: `src/backend/prealloc/select_chain_lookups.cpp:412`.
- `find_prepared_scalar_select_chain_materialization`: primary overload at `src/backend/prealloc/select_chain_lookups.cpp:429`, query overload at `src/backend/prealloc/select_chain_lookups.cpp:467`.

Current consumers and their include paths:
- `src/backend/prealloc/call_plans.cpp:1369` calls `find_prepared_direct_global_select_chain_dependency`; it includes `prepared_lookups.hpp` at line 2.
- `src/backend/prealloc/publication_plans.cpp:747` calls `find_prepared_select_chain_source_producer`, line 754 calls `find_prepared_store_source_direct_global_select_chain_dependency`, and line 951 calls `find_prepared_select_chain_source_producer`; it includes `publication_plans.hpp` at line 1.
- `src/backend/prealloc/prepared_printer/select_chains.cpp:193` calls `find_prepared_scalar_select_chain_materialization`; it includes `../prepared_lookups.hpp` at line 3 and `../publication_plans.hpp` at line 4.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:267` and line 286 call `prepare::find_prepared_direct_global_select_chain_dependency`; line 268 and line 287 construct `prepare::PreparedSelectChainDependencyQuery`; it includes `../../../prealloc/prepared_lookups.hpp` at line 9, and `dispatch_producers.hpp` also includes that header at `src/backend/mir/aarch64/codegen/dispatch_producers.hpp:5`.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp:85` calls `prepare::find_prepared_scalar_select_chain_materialization`; line 86 constructs `prepare::PreparedSelectChainDependencyQuery`; it includes `../../../prealloc/prepared_lookups.hpp` at line 20 and reaches publication-plan declarations through `select_materialization.hpp`, which includes `../../../prealloc/publication_plans.hpp` at `src/backend/mir/aarch64/codegen/select_materialization.hpp:4`.
- `src/backend/mir/aarch64/codegen/alu.cpp:2232` calls `prepare::find_prepared_scalar_select_chain_materialization`; it includes `../../../prealloc/prepared_lookups.hpp` at line 16 and reaches publication-plan declarations through `select_materialization.hpp`.
- `src/backend/mir/aarch64/codegen/memory.cpp:3907` calls `prepare::find_prepared_store_source_direct_global_select_chain_dependency`; it includes `../../../prealloc/prepared_lookups.hpp` at line 22 and reaches publication-plan declarations through `select_materialization.hpp`.
- `src/backend/mir/aarch64/codegen/calls.cpp:7862` calls `prepare::find_prepared_direct_global_select_chain_dependency`; it has no direct prealloc lookup include in its first include block and currently reaches the declaration through `select_materialization.hpp`, which includes `publication_plans.hpp`.
- Internal uses inside `src/backend/prealloc/select_chain_lookups.cpp`: line 137 calls `find_prepared_select_chain_source_producer`, line 373 calls it again, line 403 calls the primary direct-global overload, line 420 calls the primary direct-global overload, line 445 calls source-producer lookup, line 451 calls direct-global lookup, and line 474 calls the primary scalar materialization overload.

Current broad include surface:
- `prepared_lookups.hpp` direct includes: `prepared_lookups.cpp:1`, `decoded_home_storage.hpp:3`, `module.hpp:18`, `call_plans.cpp:2`, `publication_plans.hpp:8`, `formal_publications.cpp:3`, AArch64 files `module/module.cpp:4`, `module/module.hpp:6`, `codegen/dispatch_lookup.cpp:4`, `dispatch_edge_copies.cpp:18`, `memory.cpp:22`, `dispatch_producers.cpp:9`, `dispatch_publication.cpp:4`, `memory_store_retargeting.cpp:6`, `dispatch_producers.hpp:5`, `fp_value_materialization.cpp:16`, `dispatch_value_materialization.cpp:20`, `globals.cpp:2`, `comparison.cpp:9`, `frame_slot_address.cpp:4`, `alu.cpp:16`, and `select_materialization.cpp:11`.
- `publication_plans.hpp` direct includes: `module.hpp:19`, `publication_plans.cpp:1`, `dispatch_edge_copies.cpp:19`, `select_materialization.hpp:4`, and `dispatch_edge_copies.hpp:4`.

Include-cycle risks:
- `publication_plans.hpp` already includes `prepared_lookups.hpp` at line 8. Moving the remaining declarations into `prepared_lookups.hpp` would avoid a syntactic cycle but would keep the lookup facade as the owner and grow it with store/scalar select-chain facts.
- Making `publication_plans.hpp` the stable owner would also avoid a syntactic cycle, but would force call-planning and AArch64 users to depend on the publication-planning facade for general select-chain lookup facts.
- A new `select_chain_lookups.hpp` must not include `publication_plans.hpp`; if `publication_plans.hpp` then includes the new owner, that would create an owner cycle. The safe shape is for the new header to own the select-chain-specific result alias/structs currently adjacent to the declarations in `publication_plans.hpp` (`PreparedStoreSourceDirectGlobalSelectChainDependency`, `PreparedScalarSelectChainMaterialization`) and use minimal includes/forward declarations such as `calls.hpp`, `names.hpp`, `bir::Block`, `bir::Value`, `PreparedNameTables`, `PreparedEdgePublicationSourceProducer`, and `PreparedEdgePublicationSourceProducerLookups`.

Recommendation: use a new paired owner header, `src/backend/prealloc/select_chain_lookups.hpp`, paired with `select_chain_lookups.cpp`. That boundary matches the current implementation owner, keeps the API target-neutral, avoids promoting publication planning as the dependency owner for AArch64/call-planning consumers, and avoids widening `prepared_lookups.hpp` further. The next edit should move the query struct, the five target declarations, and the select-chain-specific result alias/structs needed by those declarations to the new header, then make existing consumers include it directly where they use these names.

## Suggested Next

Start plan Step 2 by adding `src/backend/prealloc/select_chain_lookups.hpp` as the selected owner, moving only select-chain declarations/result types needed for that API, and updating `select_chain_lookups.cpp` to include the paired header without changing semantics.

## Watchouts

Do not make `select_chain_lookups.hpp` include `publication_plans.hpp`; move the select-chain-specific result alias/structs out instead. Watch `calls.cpp`, which currently gets the direct-global lookup declaration transitively through `select_materialization.hpp`/`publication_plans.hpp`, and make it an explicit consumer during include cleanup.

## Proof

Analysis-only inspection packet. No build/test command was required, and no `test_after.log` was created or modified by this packet.
