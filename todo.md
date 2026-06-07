Status: Active
Source Idea Path: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Expose Edge-Publication And Parallel-Copy Prepared Facts

# Current Packet

## Just Finished

Step 2 - Expose Edge-Publication And Parallel-Copy Prepared Facts complete
across two compact sub-slices.

Sub-slice 1, prior committed packet: moved the lowest-risk local AArch64
edge-copy predicate into shared prepared lookup authority:

- Added public
  `prepare::prepared_edge_copy_source_facts_have_materializable_producer`.
- Implemented it over `PreparedEdgeCopySourceFactsStatus::Available`,
  publication authority, and non-`Unknown`/non-`Immediate` producer kind.
- Replaced the local `dispatch_edge_copies.cpp` helper and all three AArch64
  edge-copy call sites with the shared prepared helper.
- Added focused prepared lookup helper coverage for materializable load-local
  facts and fail-closed immediate, unknown, unavailable, and publicationless
  facts.

Sub-slice 2, current packet: moved one additional compact AArch64
edge-publication validation fact into shared prepared lookup authority:

- Added public
  `prepare::prepared_edge_publication_source_home_matches_source`.
- Added public
  `prepare::prepared_edge_publication_source_memory_matches_access`.
- Replaced the local AArch64 `dispatch_edge_copies.cpp` source-home and
  source-memory consistency helpers with the shared prepared predicates.
- Kept AArch64 register parsing, load-instruction matching, lowering context
  assembly, and machine-register hazard checks local to AArch64 lowering.
- Added focused prepared lookup helper coverage for matching and mismatched
  source homes, matching source memory, mismatched source-memory offsets, and
  unnamed prepared memory access results.
- Extended AArch64 dispatch coverage so direct load edge publications fail
  closed when the prepared publication memory fact no longer matches the
  prepared access.

Retained Step 1 characterization summary:

- Helper-to-contract map: edge-copy lowering had local prepared block/context
  resolution, producer pointer/instruction validation, named source-producer
  lookup wrapping, source-home/source-memory consistency checks, and the
  materializable-producer predicate. `dispatch_producers.cpp` had analogous
  current-block producer/join-routing residue plus AArch64-local
  register-alias consumers.
- Existing shared prepared queries: `make_prepared_edge_publication_source_producer_lookups`,
  `find_indexed_prepared_edge_publication_source_producer`,
  `find_prepared_same_block_scalar_producer`,
  `find_prepared_fused_compare_operand_producer`,
  `find_prepared_materialized_condition_producer`,
  `find_prepared_global_load_access`,
  `find_prepared_same_block_global_load_access`,
  `find_prepared_same_block_load_local_stored_value_source`,
  `prepare_edge_copy_source_facts`,
  `prepare_block_entry_parallel_copy_edge_source_facts`,
  `prepared_edge_publication_matches_parallel_copy_move_source`,
  `prepared_edge_publication_redundant_block_entry_parallel_copy_move`,
  `prepare_current_block_join_parallel_copy_source_facts`,
  `collect_prepared_block_entry_publications`, and
  `prepared_block_entry_publication_available`.
- Candidate missing or too-private facts: public edge-copy materializable
  source-producer predicate, prepared producer-instruction/context validation,
  source-home/source-memory consistency predicates, current-block join query
  input/routing helper, and possibly select-specific same-block producer facts.
- Step 1 proof recommendations: use `backend_prepared_lookup_helper` for
  shared edge-publication/parallel-copy and current-block join facts; pair with
  `backend_aarch64_instruction_dispatch` for AArch64 edge-publication behavior;
  rerun the supervisor-selected fresh build/proof after header or shared
  prepared lookup changes.

## Suggested Next

Delegate the next packet as Step 3 producer-context cleanup only if the
remaining boundary is explicitly about target-neutral prepared producer
instruction/context validation. Otherwise request plan-owner review before
moving more AArch64 edge-copy logic.

## Watchouts

- This packet deliberately did not touch `dispatch_producers.cpp`.
- No predecessor rescans, BIR-name matching, named-case shortcuts, or
  expectation downgrades were added.
- `clang-format` is not installed in this environment; formatting was kept
  manual and local.
- The remaining local AArch64 edge-copy helpers depend on BIR instruction
  pointers, AArch64 register parsing, lowering contexts, or recursive hazard
  checks; moving them wholesale would no longer be a compact Step 2 helper.
- The new shared memory predicate compares already-prepared publication facts
  against a supplied `PreparedMemoryAccess`; it does not discover memory
  accesses or rescan predecessors.

## Proof

Ran exactly:

`(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and both targeted
tests passing:

- `backend_aarch64_instruction_dispatch`
- `backend_prepared_lookup_helper`
