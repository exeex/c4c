Status: Active
Source Idea Path: ideas/open/96_aarch64_calls_deferred_move_publication_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish Deferred Cluster Map

# Current Packet

## Just Finished

Step 1 - Establish Deferred Cluster Map completed as an audit-only pass. The
current deferred clusters from idea 92 and the active source idea map to these
concrete entry points:

- Before-call move bundle lowering:
  - likely prepared fact producers: `prepare::populate_call_plans`,
    `prepare::select_prepared_call_argument_source`,
    `prepare::find_before_call_argument_move`,
    `prepare::classify_prepared_call_boundary_move`,
    `prepare::plan_prepared_call_boundary_effects`, and the
    `PreparedMovePhase::BeforeCall` lookup via `find_move_bundle`
  - dispatch consumers: `dispatch_prepared_block` calls
    `lower_before_call_moves`, then
    `retarget_call_boundary_source_to_emitted_scalar`,
    `materialize_call_boundary_source_to_destination`,
    `record_call_boundary_destination`, and
    `record_call_boundary_source_in_destination`
  - AArch64 emission sites: `lower_before_call_moves`,
    `lower_before_call_move`, `lower_before_call_immediate_binding`,
    `make_call_boundary_machine_instruction`,
    `make_call_boundary_move_instruction`,
    `make_call_boundary_abi_binding_instruction`,
    `make_value_stack_move_instruction`,
    `make_byval_register_lane_stack_publication_instruction`,
    `materialize_call_boundary_source_to_destination`, and
    `make_select_chain_materialization_instruction`

- After-call, return, value, and preservation lowering:
  - likely prepared fact producers: `prepare::populate_call_plans`,
    `prepare::plan_prepared_call_boundary_effects`,
    `prepare::append_explicit_call_boundary_effects`,
    `prepare::append_preservation_call_boundary_effects`,
    `prepare::make_result_source_endpoint`,
    `prepare::make_result_destination_endpoint`,
    `prepare::make_preserved_storage_endpoint`,
    `prepare::make_preserved_republication_endpoint`, and
    `PreparedMovePhase::AfterCall` / return value move bundles through
    `find_move_bundle`
  - dispatch consumers: `dispatch_prepared_block` calls
    `record_call_result_source_register`, `lower_after_call_moves`,
    `record_call_boundary_destination`,
    `retarget_fpr_call_result_store_value_to_emitted_scalar`, and
    `lower_before_return_moves`; block/value move handling also routes through
    `lower_value_moves`
  - AArch64 emission sites: `lower_after_call_moves`,
    `lower_after_call_move`, `lower_before_return_moves`, `lower_value_moves`,
    `make_callee_saved_preservation_home_population`,
    `make_callee_saved_preservation_home_republication_instruction`,
    `make_call_boundary_machine_instruction`,
    `make_call_boundary_move_instruction`, and
    `make_call_boundary_abi_binding_instruction`

- Scalar producer dispatch bridge:
  - likely prepared fact producers: `prepare::populate_call_plans`,
    `prepare::make_prepared_edge_publication_source_producer_lookups`,
    `prepare::plan_prepared_scalar_publication`,
    `prepare::plan_prepared_store_source_publication`, and prepared source
    producer lookup helpers in `publication_plans.cpp`
  - dispatch consumers: `dispatch_prepared_block` calls
    `lower_scalar_call_argument_producers`; branch fusion hooks use
    `emit_value_publication_to_register` and
    `prepared_publication_source_producer_for_value`; producer tracing also
    passes through `prepared_same_block_publication_source_producer`,
    `prepared_source_producer_instruction`, and
    `prepared_select_chain_contains_direct_global_load`
  - AArch64 emission sites: `lower_scalar_call_argument_producers`,
    `materialize_scalar_call_argument_value`,
    `materialize_local_aggregate_address_call_argument`,
    `make_scalar_call_argument_immediate`,
    `make_byval_register_lane_prepared_source`,
    `emit_value_publication_to_register`, and indirect-callee helpers
    `find_prepared_indirect_callee_source_producer`,
    `find_prepared_indirect_callee_direct_global_select_chain`,
    `find_prepared_indirect_callee_stored_value_source`,
    `emit_indirect_callee_value_to_register_with_csel`, and
    `materialize_indirect_call_callee_to_prepared_register`

- Result recording and late publication:
  - likely prepared fact producers: `prepare::populate_call_plans`,
    `prepare::find_prepared_call_plans`,
    `prepare::first_indexed_stack_preserved_values_for_call`,
    `prepare::find_indexed_prepared_value_home`,
    `prepare::PreparedCallPlan::results`,
    `prepare::PreparedCallPlan::preserved_values`, and prepared publication
    source producer lookups from `publication_plans.cpp`
  - dispatch consumers: `dispatch_prepared_block` calls
    `record_call_result_source_register`,
    `materialize_missing_frame_slot_call_arguments`,
    `publish_stack_preserved_call_values`, and late
    `retarget_fpr_call_result_store_value_to_emitted_scalar`
  - AArch64 emission sites: `record_call_result_source_register`,
    `retarget_fpr_call_result_store_value_to_emitted_scalar`,
    `materialize_missing_frame_slot_call_arguments`,
    `publish_stack_preserved_call_values`,
    `frame_slot_address`,
    `make_select_chain_materialization_instruction`, and
    `emit_value_publication_to_register`

## Suggested Next

Trace Step 2 next: start with before-call move bundle lowering from
`prepare::plan_prepared_call_boundary_effects` /
`prepare::classify_prepared_call_boundary_move` into `lower_before_call_moves`,
then through `dispatch_prepared_block` source retargeting/materialization and
destination recording. This is the best first trace because its producer,
consumer, and AArch64 emission sites are now the most directly bounded.

## Watchouts

- This is an audit-only plan until a later step creates narrow follow-up ideas.
- Do not edit implementation or test expectation files during audit packets.
- Do not treat AArch64 scratch choice, call spelling, materialization spelling,
  or machine-record construction as shared authority without traced evidence.
- Do not reopen ideas 93-95 as local-owner routes.
- `dispatch_prepared_block` is the dispatch-state mutation hub; it records
  call-boundary destinations/sources and call results around call emission, so
  later traces should distinguish mutation of `BlockScalarLoweringState` from
  prepared fact consumption.
- `emit_value_publication_to_register` appears in both dispatch publication and
  calls paths; classify its AArch64 materialization spelling separately from
  the prepared publication/source-producer facts that feed it.
- Indirect-callee materialization uses prepared source producers but also owns
  target-local scratch choice and concrete `csel`/materialization emission.

## Proof

Audit-only packet. No build or backend test proof required, and no
`test_after.log` was generated because no implementation or expectation files
were touched.
