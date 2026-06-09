Status: Active
Source Idea Path: ideas/open/141_prepared_lookups_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory residual prepared lookup public groups

# Current Packet

## Just Finished

Step 1 inventory completed for `src/backend/prealloc/prepared_lookups.hpp`.
The remaining public declarations group into the following residual surfaces:

| Group | Public declarations | Initial ownership question | Target coverage |
| --- | --- | --- | --- |
| Stack-layout id lookup helpers | `find_frame_slot_by_id`, `find_stack_object_by_id` | Should these generic stack-layout id queries live with `stack_layout/stack_layout.hpp` or stay in the central prepared lookup facade because current consumers reach them through aggregate prealloc helpers? | Additional residual public surface; not named in the source idea target list. |
| Move-bundle index facts and maps | `PreparedAfterCallResultLaneBinding`, `PreparedMoveBundleLookups` fields `bundles_by_position`, `before_call_argument_moves_by_position_and_abi`, `before_return_abi_moves_by_source_and_bank`, `after_call_result_lane_bindings`, `after_call_result_lane_bindings_by_position_and_value` | Are these core cached indexes for `PreparedFunctionLookups`, or should call/return ABI-specific pieces move toward `calls.hpp` while generic move-bundle lookup storage remains central? | Explicit source-idea target: move-bundle lookup indexes. |
| Return-chain index maps | `PreparedReturnChainLookups` fields `terminal_return_values_by_chain_value`, `next_operand_values_by_chain_value` | Is return-chain lookup construction a central control-flow/value-name cache, or does the chain semantics have a clearer owner under control-flow or return planning? | Explicit source-idea target: return-chain lookup indexes. |
| Value-home index maps | `PreparedValueHomeLookups` fields `homes_by_id`, `value_ids` | Should value-home reverse indexes move to `value_locations.hpp` ownership, or remain in `prepared_lookups.*` because multiple residual query groups consume them through `PreparedFunctionLookups`? | Explicit source-idea target: value-home lookup indexes. |
| Current-block join source status naming | `PreparedCurrentBlockJoinParallelCopySourceStatus`, `prepared_current_block_join_parallel_copy_source_status_name` | Is this status domain-owned by publication/control-flow join-copy facts, or is it specifically tied to the AArch64 routing convenience noted by the existing header comment? | Explicit source-idea target: current-block join parallel-copy source facts and routing conveniences. |
| Current-block entry publication status naming | `PreparedCurrentBlockEntryPublicationStatus`, `prepared_current_block_entry_publication_status_name` | Should entry-publication availability/status reporting live with `publication_plans.hpp`, or is it a facade-level query result because it joins names, value homes, value locations, and publication plans? | Explicit source-idea target: current-block entry publication lookup. |
| Same-block producer query facts | `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, `PreparedCurrentBlockPublicationConsumption` | Is this a reusable same-block materialization query API that deserves a narrow owner, or should it remain beside publication source-producer lookup indexes because it interprets those prepared facts? | Explicit source-idea target: same-block scalar producer and integer-constant materialization queries. |
| Call-argument materialization facts | `PreparedCallArgumentSourceProducerMaterialization`, `prepared_call_argument_binary_producer_opcode_is_materializable`, `find_prepared_call_argument_source_producer_materialization` | Should call-argument materialization policy move under `calls.hpp`, or does its dependency on same-block scalar producers make it part of a broader same-block materialization owner? | Explicit source-idea target: call-argument source producer materialization queries. |
| Fused-compare and condition producer facts | `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`, `PreparedMaterializedConditionProducer`, `find_prepared_fused_compare_operand_producer`, `find_prepared_fused_compare_operand_producer_facts`, `find_prepared_materialized_condition_producer` | Are these comparison-domain facts that should move toward a comparison owner, or shared same-block producer facts that AArch64 comparison happens to consume first? | Explicit source-idea target: fused-compare and materialized-condition producer facts. |
| Same-block load-local stored-value facts | `PreparedSameBlockLoadLocalStoredValueSource`, `find_prepared_same_block_load_local_stored_value_source` | Should this live with addressing/memory-access ownership because it joins stack layout and prepared memory accesses, or with same-block materialization because it discovers a producer value before the current instruction? | Explicit source-idea target: same-block load-local stored-value source facts. |
| Current-block join parallel-copy source facts | `PreparedCurrentBlockJoinParallelCopySourceFact`, `PreparedCurrentBlockJoinParallelCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`, `prepare_current_block_join_parallel_copy_source_facts` | Which fields are reusable out-of-SSA/publication facts, and which fields are target-facing routing convenience such as register-name sharing, stack-source flags, and immediate-source flags that may need a split? | Explicit source-idea target and explicit idea-140 revisit target. |
| Current-block entry publication query result | `PreparedCurrentBlockEntryPublicationQueryInputs`, `PreparedCurrentBlockEntryPublication`, both `find_prepared_current_block_entry_publication` overloads | Does this belong with publication plans as a publication lookup, or with value-home/value-location lookup aggregation because it resolves destination homes and value ids for the current block? | Explicit source-idea target: current-block entry publication lookup. |
| Current-block join instruction routing convenience | `PreparedCurrentBlockJoinParallelCopyInstructionRouting`, `prepare_current_block_join_parallel_copy_instruction_routing` | Is this still reusable prepared analysis, or should it be split from shared facts because it answers a target-routing question about which instruction results need join-copy handling? | Explicit source-idea target and explicit idea-140 revisit target. |
| Function-level lookup aggregate | `PreparedFunctionLookups` fields `call_plans`, `address_materializations`, `memory_accesses`, `move_bundles`, `return_chains`, `value_homes`, `edge_publications`, `edge_publication_source_producers`; `make_prepared_function_lookups` | Should `PreparedFunctionLookups` remain the long-term central prepared-query aggregate while narrow domain headers own their own lookup structs, or should this facade shrink to construction wiring only? | Explicit source-idea target: value-home lookup indexes and `PreparedFunctionLookups`; also covers already-split idea 137-140 aggregate fields. |
| Key helper functions | `prepared_move_bundle_position_key`, `prepared_after_call_result_lane_position_key`, `prepared_before_return_abi_move_source_bank_key`, `prepared_return_chain_value_key` | Are these implementation details that can become private to the owning lookup builder, or are they intentionally public stable key constructors for consumers outside `prepared_lookups.cpp`? | Additional residual public surface; supports explicit move-bundle and return-chain targets. |
| Lookup preparation helpers | `make_prepared_move_bundle_lookups`, `make_prepared_return_chain_lookups`, `make_prepared_value_home_lookups`, both `make_prepared_edge_publication_lookups` overloads, `make_prepared_edge_publication_source_producer_lookups` | Which builders are central aggregate construction helpers, and which should be exported by existing domain owners such as publication plans, value locations, or control-flow? | Explicit for move-bundle, return-chain, value-home, and aggregate targets; edge-publication builders are residual surface from prior idea 140 work. |
| Value-home and out-of-SSA helper predicates | `prepared_value_homes_share_register_name`, `prepared_out_of_ssa_parallel_copy_register_destination_matches_value`, `prepared_out_of_ssa_parallel_copy_source_shares_destination_register` | Are these generic value-home/out-of-SSA predicates that belong with value-location/control-flow data, or are they coupled to join parallel-copy routing and therefore candidates for a fact/routing split? | Additional residual public surface; supports explicit current-block join parallel-copy target. |
| Indexed move-bundle query helpers | `find_indexed_prepared_move_bundle`, `find_indexed_prepared_before_call_argument_move`, `find_indexed_prepared_after_call_result_lane_binding`, `find_prepared_before_return_abi_move_by_source_and_destination_bank` | Should these query helpers remain public with `PreparedMoveBundleLookups`, or should call argument and return ABI variants move to call/return owners while the raw bundle lookup stays central? | Explicit source-idea target: move-bundle lookup indexes. |
| Return-chain query helpers | `find_prepared_return_chain_terminal_value`, `find_prepared_return_chain_next_operand_value` | Should these remain facade-level return-chain index lookups, or move under whichever owner owns return-chain construction after definition mapping? | Explicit source-idea target: return-chain lookup indexes. |
| Publication source-producer lookup helper | `find_indexed_prepared_edge_publication_source_producer` | Is this leftover facade surface from idea 140 that should move with `PreparedEdgePublicationSourceProducerLookups`, or is it acceptable as a central lookup accessor used by several same-block query groups? | Additional residual public surface tied to prior edge-publication split. |
| Current-block publication consumption query | `find_prepared_current_block_publication_consumption` | Should this query live with publication source-producer ownership, or with same-block materialization because it is the common first step for local producer materialization? | Explicit source-idea target through same-block scalar producer/materialization queries. |
| Same-block scalar and integer-constant query helpers | `find_prepared_same_block_scalar_producer` overloads, `evaluate_prepared_same_block_integer_constant` overloads | Should these form a narrow same-block materialization API, or remain in `prepared_lookups.*` because they are cross-cutting helpers over names, source producers, blocks, and instruction indices? | Explicit source-idea target: same-block scalar producer and integer-constant materialization queries. |
| Stack-source publication helpers absent from this header | No same-width stack-source or aggregate stack-source publication helper declarations appear in `prepared_lookups.hpp`; the only stack-source-related public surface here is `PreparedSameBlockLoadLocalStoredValueSource` and join-source flags such as `source_home_is_stack` | Later steps must inspect `publication_plans.*` and consumers to answer the source idea's same-width and aggregate stack-source publication-helper question; Step 1 finds no standalone public group for those helpers in this header. | Explicit source-idea revisit target, but not a distinct `prepared_lookups.hpp` public declaration group. |

## Suggested Next

Execute Step 2 from `plan.md`: map each inventory group to definitions and
construction wiring in `src/backend/prealloc/prepared_lookups.cpp`, noting
whether the implementation is central lookup construction, domain semantic
logic, or target-facing routing convenience.

## Watchouts

- This active idea is analysis-only; do not edit implementation files.
- Step 1 covered only `prepared_lookups.hpp`; do not treat the initial owner
  questions above as final classifications until definitions and consumers are
  mapped.
- Same-width stack-source and aggregate stack-source publication helpers are
  source-idea targets, but they are not standalone public declarations in this
  header.
- Do not use line count alone as evidence that a residual group should move.
- Do not propose deleting reusable prepared facts or replacing them with local
  BIR rescans, predecessor rescans, or name matching.
- Keep target-local AArch64 routing, register, scratch, hazard, and emission
  policy out of shared prealloc follow-up proposals.
- Do not reopen ideas 137-140 unless a concrete residual dependency proves an
  unresolved owner boundary.

## Proof

Inspection-only analysis slice. Read `prepared_lookups.hpp`,
`ideas/open/141_prepared_lookups_residual_owner_audit.md`, and `plan.md`; no
build or test command was run, and no `test_after.log` was required by the
delegated packet.
