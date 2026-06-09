Status: Active
Source Idea Path: ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Audit Inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 1: established the prepared/prealloc audit inventory
and ideas 130-135 closure context for later classification. Inspection used
`rg` plus direct reads of the active plan, `todo.md`, closed ideas 130-135, and
the relevant prealloc headers/source files. AST-backed lookup was not needed for
this analysis-only inventory because the public query surfaces are declared in
headers and the target source files were inspected by declaration/search.

### Ideas 130-135 Closure Context

| Idea | Closure context relevant to this audit |
| --- | --- |
| 130 `aarch64_dispatch_family_post_contract_layout_audit` | Analysis-only dispatch audit after earlier prepared producer, edge-publication, current-block, calls, memory, wide-value, and i128-shift work. It retained broad AArch64 public hooks where they were real cross-file contracts, rejected file-count-only cleanup, and created ideas 131-135 for concrete stale public surface or shared-query gaps. It explicitly marked shared-query candidates around edge producers/publication, prepared value homes, current-block publication, same-block materialization, and select-chain dependencies. |
| 131 `aarch64_dispatch_edge_copy_helper_surface_privatization` | Closed after narrowing `dispatch_edge_copies.hpp` to retained cross-file orchestration hooks and privatizing edge-copy producer/publication helpers in `dispatch_edge_copies.cpp`. No shared prepared query was introduced; this was AArch64 helper-surface cleanup that preserved edge-copy behavior and kept edge-copy emission target-local. |
| 132 `aarch64_dispatch_lookup_thin_helper_surface_trim` | Closed after trimming `dispatch_lookup.hpp` to `make_named_prepared_result_register` and `emitted_scalar_value_available`. No broader shared-query work was folded in; the retained hooks remain AArch64 lookup contracts used by non-dispatch lowering code. |
| 133 `shared_prepared_fact_query_surface_extraction` | Closed after adding `prepare::find_prepared_value_home_for_bir_value(...)` and routing AArch64 rediscovery sites in dispatch publication, select materialization, comparison, and ALU through that target-neutral prepared value-home surface. It intentionally deferred indexed-home, return-chain, move-bundle, call-plan, memory-access, preserved-value, emitted-register, edge-publication, select-chain, memory-access, and call-plan relationship surfaces as distinct query domains. |
| 134 `shared_select_chain_same_block_dependency_queries` | Closed after exposing same-block materialization and select-chain dependency relationships through shared prepared query facades and routing AArch64 callers through them while keeping emission, hazards, scratch choice, and materialization order local. Relevant surfaces include same-block scalar/integer/materialized-condition/fused-compare producers and direct-global select-chain dependency queries. |
| 135 `shared_current_block_entry_publication_query` | Closed after adding a shared current-block entry publication query in the prepared/prealloc surface and routing the selected AArch64 publication register lookup through it. AArch64 register-view selection, publication register recording, branch-fusion hooks, operand spelling, and final emission stayed local. |

### Domain-Grouped Query Inventory

| Domain | Current shared/prealloc query groups | Primary owner files | Provenance / later classification notes |
| --- | --- | --- | --- |
| Value-home and prepared storage lookup | `PreparedValueHomeLookups`, `make_prepared_value_home_lookups`, `find_prepared_value_home_for_bir_value` in `value_locations.hpp`, `find_indexed_prepared_value_home`, `find_indexed_prepared_value_id`, `prepared_value_homes_share_register_name`, `find_frame_slot_by_id`, `find_stack_object_by_id`, `PreparedDecodedHomeStorage`/`decode_prepared_value_home`, scalar publication home/storage helpers. | `prepared_lookups.hpp/.cpp`, `value_locations.hpp`, `decoded_home_storage.*`, `publication_plans.*` | Idea 133 introduced the BIR-value-to-home shared query and explicitly deferred several indexed or adjacent storage relationships. Later steps should decide whether index lookups and decode/publication helpers are clean shared facts or overly broad facade members. |
| Current-block entry publication | `PreparedCurrentBlockEntryPublicationQueryInputs`, `PreparedCurrentBlockEntryPublication`, `find_prepared_current_block_entry_publication(...)`, `collect_prepared_block_entry_publications`, `prepared_block_entry_publication_available`, AArch64 wrappers around `value_has_current_block_entry_publication` and publication clobber checks. | `prepared_lookups.hpp/.cpp`, `value_locations.hpp`, AArch64 `dispatch_publication.*` and `dispatch_producers.*` consumers | Idea 135 added the shared query. Later classification should separate target-neutral availability/home lookup from AArch64 register-index parsing and clobber policy. |
| Edge-publication and move-bundle facts | `PreparedEdgePublicationLookups`, `PreparedEdgePublicationKey`, `make_prepared_edge_publication_lookups`, `find_indexed_prepared_edge_publications`, `find_unique_indexed_prepared_edge_publication`, `prepare_edge_copy_source_facts`, `find_unique_indexed_block_entry_parallel_copy_edge_publication`, `prepare_block_entry_parallel_copy_edge_source_facts`, edge-copy/source status enums, `PreparedMoveBundleLookups`, `make_prepared_move_bundle_lookups`, `find_indexed_prepared_move_bundle`, before-call/after-call/before-return move queries, out-of-SSA parallel-copy helpers in `control_flow.hpp`. | `prepared_lookups.hpp/.cpp`, `control_flow.hpp`, `out_of_ssa.cpp`, AArch64 `dispatch_edge_copies.*` and `dispatch_producers.*` consumers | Idea 131 privatized AArch64 edge-copy helpers but did not add shared queries. Existing edge-publication and move-bundle APIs are major candidates for Step 2 ownership/consumer mapping because they span shared source facts, move resolution, and target-local edge emission. |
| Same-block producer and materialization facts | `PreparedEdgePublicationSourceProducerLookups`, `make_prepared_edge_publication_source_producer_lookups`, `find_indexed_prepared_edge_publication_source_producer`, `find_prepared_current_block_publication_consumption`, `find_prepared_same_block_scalar_producer`, `evaluate_prepared_same_block_integer_constant`, `find_prepared_fused_compare_operand_producer`, `find_prepared_fused_compare_operand_producer_facts`, `find_prepared_materialized_condition_producer`, `find_prepared_same_block_global_load_access`, `find_prepared_same_block_load_local_stored_value_source`, `find_prepared_same_block_load_local_source_producer`. | `prepared_lookups.hpp/.cpp`, `publication_plans.hpp/.cpp`, AArch64 `dispatch_producers.*`, `dispatch_value_materialization.*`, `select_materialization.cpp`, comparison/ALU consumers | Idea 134 is the main provenance. These are intended as target-neutral prepared relationships; later classification should check for duplicate overload shapes between `prepared_lookups.hpp` and `publication_plans.hpp`. |
| Select-chain dependency facts | `PreparedDirectGlobalSelectChainDependency`, `PreparedSelectChainDependencyQuery`, `find_prepared_direct_global_select_chain_dependency(...)` overloads in both `prepared_lookups.hpp` and `publication_plans.hpp`, `find_prepared_select_chain_source_producer`, `find_prepared_store_source_direct_global_select_chain_dependency`, `find_prepared_scalar_select_chain_materialization`, call argument direct-global select-chain dependency accessors in `calls.hpp`. | `prepared_lookups.hpp/.cpp`, `publication_plans.hpp/.cpp`, `calls.hpp`, `call_plans.cpp` | Idea 134 generalized select-chain/same-block dependency queries; call and store-source variants may be good domain-specific projections or duplicated narrow surfaces. Step 3 should classify them after consumer mapping. |
| Call-plan argument, result, outgoing-stack, and boundary facts | `PreparedCallPlanLookups`, `make_prepared_call_plan_lookups`, `find_indexed_prepared_call_plan`, `find_indexed_prepared_immediate_call_argument`, `find_indexed_prepared_outgoing_stack_argument_area`, `find_indexed_prepared_after_call_result_lane_binding`, `find_prepared_before_return_abi_move_by_source_and_destination_bank`, `find_prepared_call_argument_publication_source_routing`, `find_prepared_call_argument_direct_global_select_chain_dependency`, `find_prepared_missing_frame_slot_call_argument_publication_need`, `find_prepared_call_result_late_publication`, `prepared_call_boundary_move_classification_available`, `plan_prepared_call_boundary_effects`, `indexed_block_entry_republication_effects_for_block`. | `prepared_lookups.hpp/.cpp`, `calls.hpp`, `call_plans.cpp` | Idea 133 explicitly deferred call-plan relationships. These are current audit inventory items because they are prepared query surfaces, not because prior closure proved they are smells. |
| Memory-access and pointer/value-home facts | `PreparedAddressMaterializationLookups`, `PreparedMemoryAccessLookups`, `make_prepared_address_materialization_lookups`, `make_prepared_memory_access_lookups`, `find_indexed_prepared_address_materializations`, `find_indexed_prepared_memory_access`, result-name/result-id memory access queries, `find_indexed_prepared_frame_address_offset_for_value(_id)`, `collect_prepared_address_materializations_for_block`, `find_prepared_global_load_access`, store-source publication planners, fixed formal publication, recovered narrow-store source, byval formal pointer-source detection, addressing inline lookups in `addressing.hpp`. | `prepared_lookups.hpp/.cpp`, `addressing.hpp`, `publication_plans.hpp/.cpp`, `call_plans.cpp` | Idea 133 deferred memory-access relationships. Later classification should separate reusable memory/address facts from publication planner policy and AArch64-specific uses. |
| Preservation and runtime-helper facts | Prior preservation queries: `find_latest_indexed_prior_preserved_value`, `find_dominating_indexed_prior_preserved_value`, `find_unique_indexed_prior_preserved_value_source`, `find_latest_indexed_prior_stack_preserved_value_before_instruction`, `first_indexed_stack_preserved_values_for_call`; runtime-helper availability queries: `prepared_f128_runtime_helper_has_abi_contract`, `prepared_f128_runtime_helper_has_scalar_cmp_result_bridge_contract`, `prepared_i128_runtime_helper_has_abi_contract`; module-level `find_prepared_*runtime_helpers` and carrier lookups. | `prepared_lookups.hpp/.cpp`, `calls.hpp`, `call_plans.cpp`, `runtime_helpers.hpp`, `f128_runtime_helpers.hpp`, `i128_runtime_helpers.hpp`, `module.hpp` | Idea 133 deferred preserved-value and runtime-adjacent call-plan surfaces. These look query-like but may belong to call/runtime domains rather than the main prepared lookup facade. |
| Control-flow, join, compare, and parallel-copy facts adjacent to this audit | `find_prepared_control_flow_function`, branch/block/linear predecessor queries, compare branch target resolution, join-transfer lookup, authoritative branch/join sources, short-circuit and materialized compare join contexts, prepared parallel-copy bundle and move lookups, out-of-SSA move-bundle lookup by execution block. | `control_flow.hpp`, `out_of_ssa.cpp`, consumers across AArch64 comparison/select/dispatch paths | These are adjacent BIR/prealloc contracts discovered during inventory. They are not all in `prepared_lookups.*`, but Step 2 may need them when mapping edge-publication, select-chain, and current-block consumers. |

## Suggested Next

Execute `plan.md` Step 2: map owners and AArch64 consumers for each domain in
the inventory. Start with the current shared query facades in
`prepared_lookups.hpp`, then map call sites in AArch64 dispatch, publication,
value materialization, calls, memory, ALU, comparison, and select
materialization.

## Watchouts

- Ideas 131 and 132 were AArch64 public-surface trims; do not cite them as
  evidence that shared `prepared_lookups.*` should shrink.
- Ideas 133, 134, and 135 added shared query authority deliberately. Treat those
  APIs as suspicious only if Step 2/3 proves duplicate ownership, overly narrow
  projections, or AArch64-shaped facade leakage.
- `publication_plans.hpp` duplicates some select-chain query declarations also
  present in `prepared_lookups.hpp`; this may be an intentional public include
  boundary or a real duplicate surface.
- Several query-like surfaces live outside `prepared_lookups.*`
  (`control_flow.hpp`, `calls.hpp`, `publication_plans.hpp`, `module.hpp`,
  runtime-helper headers). Later classification should avoid over-scoping unless
  AArch64 consumers prove they are part of the same prepared/prealloc contract.
- No missing closure context blocked Step 1. The main uncertainty is provenance
  for pre-130 query surfaces; classify those by current owner/consumer evidence.

## Proof

Analysis-only inspection packet. No build/test command was required or run, and
no `test_after.log` was created for this packet.
