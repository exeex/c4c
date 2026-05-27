# AArch64 Calls Prepared Authority Repair

## Goal

Repair duplicate call authority in
`src/backend/mir/aarch64/codegen/calls.cpp` by consuming prepared call plans,
argument/result plans, source selections, preservation facts, boundary effects,
move bundles, and value homes, and by adding missing shared queries for ABI
bindings, argument moves, producer materialization, indirect callees, and
after-call results.

## Why This Exists

The audit found that `calls.cpp` already uses shared call plans for much of the
ABI boundary, but still carries local scans for immediate ABI bindings,
frame-slot argument moves, scalar call-argument producers, prepared-name BIR
sources, indirect callee sources, and after-call result registers.

## Owned File

- `src/backend/mir/aarch64/codegen/calls.cpp`

## Duplicated Helpers And Fallback Paths

- `find_prepared_call_plan`, `require_prepared_call_plan`, and
  `lower_call_instruction` must fail closed on authoritative
  `PreparedCallPlan` absence rather than falling back to raw callee analysis.
- `make_selected_call_argument_source`,
  `make_prior_preserved_call_argument_source`,
  `make_sret_memory_return_address_source`, and
  `make_selected_local_frame_address_source` should consume complete
  `source_selection` facts.
- `lower_before_call_moves`, `lower_after_call_moves`, and
  `publish_stack_preserved_call_values` should consume boundary-effect and
  preservation plans rather than rebuild ordering.
- `lower_before_call_immediate_binding` scans `call_plan.arguments` for a
  matching immediate ABI binding.
- `find_prepared_frame_slot_call_argument_move` scans a move bundle and
  reclassifies moves for one call argument.
- `materialize_scalar_call_argument_value` and
  `lower_scalar_call_argument_producers` use `find_same_block_scalar_producer`,
  recursive binary materialization, and
  `materialize_direct_global_select_chain_call_argument`.
- `materialize_call_boundary_source_to_destination` plus
  `find_bir_value_for_prepared_name` scans BIR results and call operands by
  prepared name.
- `materialize_indirect_call_callee_to_prepared_register`,
  `resolve_indirect_callee_local_load_store`, and
  `emit_indirect_callee_value_to_register_with_csel` recover indirect-callee
  producer/source facts locally.
- `record_call_result_source_register` scans after-call move bundles or
  supplements `PreparedCallResultPlan` for result publication.

## Shared Facts To Consume Or Add

- Consume `PreparedCallPlan`, `PreparedCallPlanLookups`,
  `find_indexed_prepared_call_plan`, `PreparedIndirectCalleePlan`, and
  `PreparedMemoryReturnPlan`.
- Consume `PreparedCallArgumentPlan`,
  `PreparedCallArgumentSourceSelection`, `PreparedCallPreservedValue`, and
  `PreparedMemoryReturnPlan` for argument/source decisions.
- Consume `PreparedCallBoundaryEffectPlan`, `PreparedMoveBundle`,
  `classify_prepared_call_boundary_move`,
  `plan_prepared_call_boundary_effects`, and
  `first_indexed_stack_preserved_values_for_call`.
- Add or consume a prepared call-argument lookup/classification by ABI binding
  for immediate-only bindings.
- Add or consume a prepared move lookup by call argument plan or argument ABI
  index.
- Add or consume a prepared call-argument producer/materialization query by
  argument value and call position.
- Add or consume a prepared call-boundary source value payload or
  source-producer query in `PreparedCallBoundaryEffectEndpoint`.
- Add or consume prepared indirect-callee source-producer/materialization facts,
  including select-chain and load-local substitution.
- Add or consume an after-call result lookup by destination value id and
  register bank when `PreparedCallResultPlan` alone is insufficient.

## Out Of Scope

- Do not change AAPCS64 register/stack staging, direct `bl` or indirect `blr`
  spelling, stack cleanup, source reload sequencing, or ABI result store
  instruction spelling except to consume shared facts.
- Do not implement a broad call lowering rewrite.
- Do not push call-argument producer logic into ALU or value-materialization
  follow-up ideas without an explicit shared-query contract.

## Acceptance Criteria

- Call lowering fails closed or reports a missing prepared plan instead of
  falling back to raw call operand/callee-name analysis.
- Immediate ABI binding and frame-slot argument move paths use shared argument
  binding/move lookup authority.
- Scalar call-argument and indirect-callee materialization consume shared
  producer/source-selection facts instead of recursive same-block walks.
- Call-boundary memory/source materialization no longer scans BIR values by
  prepared spelling.
- Call result publication consumes `PreparedCallResultPlan` or a shared
  after-call result lookup rather than raw after-call move-bundle scans.

## Reviewer Reject Signals

- Reject fallback to raw call operands, callee-name analysis, frame-slot names,
  or BIR result spelling when prepared call/source facts should exist.
- Reject local scans that match immediate arguments by ABI index and source
  shape as the durable binding authority.
- Reject repeated move-bundle scans for argument-specific or after-call result
  matching once shared lookups are available.
- Reject deeper recursive same-block producer walks, direct-global
  select-chain shortcuts, or indirect-callee named-case fixes.
- Reject expectation downgrades, unsupported-test rewrites, broad call rewrites
  outside this file, or helper renames claimed as capability progress.
