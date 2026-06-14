Status: Active
Source Idea Path: ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Verify Prerequisites And Inventory Field Groups

# Current Packet

## Just Finished

Step 1 read-only inventory is complete.

Prerequisite closure status: all prerequisites are closed; no prerequisite
blocks this gate.

- 243 is closed and contributes the Phase F0 convergence map: no whole
  `PreparedBirModule`, `PreparedFunctionLookups`, lookup group, or field group
  was deletion-ready, and x86/riscv parity, diagnostics, fallback,
  compatibility, wrapper output, and target-policy boundaries must be proven
  field-by-field.
- 244 is closed and contributes x86 Route 6 proof for the selected scalar `i32`
  direct-call argument path: route-native status rows cover available, missing,
  invalid, duplicate, prepared mismatch, source-name mismatch, and fallback
  behavior while `ConsumedPlans`, helper-oracle statuses, and wrapper assembly
  stay stable.
- 245 is closed and contributes riscv Route 5/Route 3 adapter proof:
  edge/source and memory/source identities are consumed only through agreement
  with prepared publication/source-memory authority, with exact instruction
  text, fail-closed statuses, and target-local register/stack/offset policy
  preserved.
- 246 is closed and contributes compatibility-retention proof: prepared
  publication, source-memory, typed-stack-source, aggregate-stack-source,
  current-block publication, helper-oracle, route-debug, fallback, and
  wrapper-output names remain explicit compatibility contracts beside
  route-native rows.

Inventoried public `PreparedFunctionLookups` field groups:

- `call_plans`: call-site, argument, result, preserved-value, boundary-effect,
  outgoing-stack, and wrapper/public `ConsumedPlans` authority relevant to
  Route 6 and x86 wrappers.
- `address_materializations`: block-keyed address-materialization lookup for
  frame slots, globals, strings, labels, GOT/TLS, byte offsets, and target
  address policy.
- `memory_accesses`: Route 3-relevant memory accesses by position, result
  value name, and result value id, including prepared address base, size,
  align, volatility, and materialization requirements.
- `move_bundles`: prepared move bundles by phase/position plus before-call
  argument moves, before-return ABI moves, and after-call result lane bindings.
- `value_homes`: prepared value id/name to home maps for register, stack,
  immediate, and pointer-base-plus-offset homes used by wrappers, fallback,
  and edge-publication consumers.
- `edge_publications`: Route 4/Route 5-relevant edge publications indexed by
  predecessor, successor, and destination value, carrying source/destination
  values, source producer, source memory, aggregate stack authority, homes,
  move/parallel-copy, carrier, and status/fallback state.
- `edge_publication_source_producers`: source-producer records by value name
  for immediate, load-local, load-global, cast, binary, and select
  materialization producers.

Inventoried `PreparedBirModule` backing field groups in scope:

- Core semantic/base state: `module`, `route`, `invariants`, `names`,
  `control_flow`, `completed_phases`, and `notes`.
- Target policy and layout state: `target_profile`, `stack_layout`,
  `regalloc`, `register_group_overrides`, `frame_plan`,
  `dynamic_stack_plan`, and `storage_plans`.
- Prepared value/address state: `value_locations`, `addressing`, and
  `liveness`.
- Route/wrapper/public lookup backing state: `call_plans`,
  `store_source_publications`, `variadic_entry_plans`, `i128_carriers`,
  `f128_carriers`, `atomic_operations`, `intrinsic_carriers`,
  `inline_asm_carriers`, `f128_runtime_helpers`, and
  `i128_runtime_helpers`.

Inventoried public helper/status compatibility surfaces:

- Route 3/source-memory: `PreparedMemoryAccessLookups`,
  `prepared_edge_publication_source_memory_access_status_name`,
  `prepared_edge_publication_source_memory_matches_access`, and riscv
  `route3_source_memory_agrees_with_prepared_publication`.
- Route 4/current-block publication: `PreparedCurrentBlockEntryPublication`,
  `PreparedCurrentBlockJoinParallelCopySourceFacts`,
  `prepared_current_block_entry_publication_status_name`, and
  `prepared_current_block_join_parallel_copy_source_status_name`.
- Route 5/edge publication: `PreparedEdgePublicationLookups`,
  `find_unique_indexed_prepared_edge_publication`,
  `PreparedEdgePublicationSourceProducerLookups`,
  `prepared_edge_copy_source_facts_status_name`, and riscv
  `route5_edge_source_agrees_with_prepared_publication`.
- Route 6/call wrapper: `PreparedCallPlanLookups`, x86 `ConsumedPlans`,
  `find_consumed_call_argument_plan`, `route6_scalar_call_argument_gate_name`,
  and the route-debug rows for `agreed`, `blocked`,
  `missing_source_value`, `missing_source_name`,
  `prepared_source_mismatch`, and `source_value_mismatch`.
- Wrapper/output/fallback helpers: riscv `EdgePublicationMoveIntentStatus`,
  `consume_edge_publication_move_intent`, `route5_edge_status`,
  `route5_edge_source_agrees`, `route3_source_memory_agrees`, and x86 direct
  call wrapper consumption through prepared call plans.
- Compatibility status families retained by 246: `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`, `publication_unavailable`,
  `unsupported_source_home`, `missing_same_width_i32_type`,
  `missing_destination_gpr_bank`, and
  `missing_aggregate_copy_authority`.

## Suggested Next

Execute Step 2: classify each inventoried field group as duplicate semantic
cache, private pass context, target-policy product, compatibility adapter,
blocked public authority, or deletion candidate. Keep the classification
field-by-field and require cross-target positive, missing, mismatch, duplicate,
fallback, wrapper-output, and baseline evidence before naming any deletion or
demotion candidate.

## Watchouts

This is a gate, not an implementation packet. The Step 1 inventory found no
prerequisite blocker, but it also did not classify readiness. Step 2 should
avoid treating x86-only Route 6, riscv-only Route 5/Route 3, diagnostic-only,
or compatibility-only proof as whole-field-group demotion evidence.

## Proof

Read-only inspection only; no build or ctest proof was required for this
inventory packet. Inspected the active plan/todo, closed ideas 243-246,
`PreparedBirModule`, `PreparedFunctionLookups`, prepared lookup constructors,
public helper/status declarations, x86 Route 6 diagnostics, riscv Route 5/Route
3 edge-publication adapter, and prepared lookup/riscv/x86 tests.
