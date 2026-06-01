# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build the helper classification table

## Just Finished

Completed Step 1 from `plan.md`: inventoried the remaining helpers in
`dispatch_publication.*`, `dispatch_value_materialization.*`, and
`dispatch_producers.*`; classified each helper family by natural owner; and
identified the first bounded relocation packet.

Classification summary:

| Helper / family | Current surface | Classification | Relocation note |
| --- | --- | --- | --- |
| `integer_bit_width` | `dispatch_publication.*` | scalar/ALU support | Keep behavior as shared scalar sizing until a scalar owner packet moves the value-publication arithmetic/cast helpers together. |
| `power_of_two_shift` | `dispatch_publication.*` | scalar/ALU support | Move with `value_power_of_two_shift` / multiply-by-power-of-two publication support, likely to `alu.cpp`. |
| `scalar_view_for_type`, `gp_register_name`, `scalar_integer_width_bits`, `scalar_gp_register_view`, `scalar_fp_register_view`, `immediate_integer_bits` | `dispatch_publication.*` | scalar/ALU register spelling support | These are broadly consumed scalar publication utilities; move only with a coherent scalar helper packet, not as isolated churn. |
| `find_frame_slot`, `find_stack_object`, `prepared_frame_slot_load_address`, `prepared_local_load_offset`, `local_aggregate_address_frame_offset`, private `prepared_frame_address_offset_for_value` | `dispatch_publication.*` | memory / stack addressing | Natural destination is `memory.cpp`/`memory.hpp` because they interpret prepared frame-slot and object addresses for local loads/stores. |
| `relocation_operand` | `dispatch_publication.*` | globals | Duplicate-style relocation formatting already exists in `globals.cpp`; global publication relocation should use the globals owner copy. |
| `emit_local_slot_address_publication_to_register`, private `emit_local_slot_address_publication_to_register_impl`, `lower_local_slot_address_publication` | `dispatch_publication.*` | memory / address materialization | Move as a stack/local-address publication packet, probably to `memory.cpp`, because the helpers publish prepared local slot addresses and optional stack spill stores. |
| `publication_parse_va_list_field_suffix`, `prepared_va_list_field_address` | `dispatch_publication.*` | calls / variadic | These are variadic-entry ABI helpers; candidate destination is the calls/variadic owner rather than dispatch. |
| `is_byval_formal_value_name` | `dispatch_publication.*` | calls | Formal ABI query; move with the fixed-formal or call-boundary publication packet if still needed there. |
| `prepared_value_home_for_value` | `dispatch_publication.*` | still-shared prepared authority | This is lookup plumbing over prepared value homes. Do not re-derive it per owner; either keep as shared helper or move only if a common prepared lookup surface already exists. |
| `collect_current_block_entry_publications`, `value_has_current_block_entry_publication`, private `current_block_entry_publication_home`, `current_block_entry_publication_register`, `record_current_block_entry_publication_registers` | `dispatch_publication.*` | current-block join publication | Keep together; natural destination is current-block join/dispatch route code because it records block-entry publication availability into scalar state. |
| `prepared_publication_source_producer_for_value`, `prepared_source_producer_instruction`, `prepared_same_block_publication_source_producer`, `find_prepared_fused_compare_operand_producer`, `preferred_fused_compare_operand_publication_target` | `dispatch_publication.*` | comparison / prepared producer lookup | These support branch-condition and fused-compare publication; move only with the fused-compare packet, likely to `comparison.cpp` or a comparison-facing hook surface. |
| `lower_missing_conditional_branch_condition_publication` | `dispatch_publication.*` | comparison | Natural owner is comparison/branch lowering because it materializes missing condition values before conditional branches. |
| `lower_missing_fused_compare_operand_publication`, `lower_missing_fused_compare_operand_publications` | `dispatch_publication.*` | comparison | Natural owner is `comparison.cpp`; depends on value publication and prepared fused-compare producer facts. |
| `record_address_materialization_result` | `dispatch_publication.*` | dispatch orchestration | This only updates dispatch scalar state after address materialization; it can remain in dispatch or move into `dispatch.cpp` with the caller loop. |
| `record_memory_result` | `dispatch_publication.*` | memory | Natural destination is `memory.cpp` because it observes `MemoryInstructionRecord` load results and records emitted scalar state. |
| `memory_load_result_feeds_before_return_fpr_abi`, private `find_storage_plan_value`, `symbol_fp_load_has_explicit_storage_placement`, `retarget_memory_result_to_prepared_home` | `dispatch_publication.*` | memory | Move as one memory-retarget packet; these adjust memory load result registers to prepared homes and return ABI facts. |
| `retarget_pointer_store_value_to_materialized_address`, `retarget_store_address_to_materialized_pointer`, `retarget_pointer_store_value_to_emitted_scalar`, `retarget_store_local_value_to_emitted_scalar` | `dispatch_publication.*` | memory | Store-local/memory record retargeting; move with memory store publication helpers. |
| `fixed_formal_scalar_store_mnemonic`, `prepared_store_local_access`, `plan_fixed_formal_store_local_publication`, `lower_fixed_formal_store_local_publication` | `dispatch_publication.*` | calls / fixed-formal publication | Formal-source publication is ABI/formal ownership; candidate destination is `calls.cpp` unless supervisor chooses memory because the emitted record is a store. |
| `block_entry_move_clobbers_current_join_publication` | `dispatch_publication.*` | current-block join / dispatch orchestration | Dispatch-loop hazard check around block-entry moves; keep with current-block join route handling. |
| `prepared_value_home_reads_register_index`, `value_publication_may_read_register_index` | `dispatch_publication.*` | scalar/ALU hazard support | Supports recursive value publication register-hazard checks; move with scalar value-publication materialization, not independently. |
| private `prepared_same_block_scalar_producer`, `prepared_scalar_select_chain_materialization`, `prepared_same_block_integer_constant` | `dispatch_value_materialization.cpp` | still-shared prepared authority for scalar publication | Query prepared producer/materialization facts; keep shared unless a scalar publication owner surface replaces the synthetic dispatch file. |
| private `emit_prepared_global_load_to_register` | `dispatch_value_materialization.cpp` | globals | Best first relocation candidate: it emits prepared same-block global loads and already uses global policy/relocation concepts. Move to `globals.cpp`/`globals.hpp`. |
| `value_power_of_two_shift`, private `value_publication_may_write_scratch_register` | `dispatch_value_materialization.cpp` | scalar/ALU | Move with arithmetic value-publication support, likely to `alu.cpp`. |
| `emit_value_publication_to_register` | `dispatch_value_materialization.*` | scalar/ALU with memory/globals/comparison/select hooks | Too mixed for a first packet. Decompose by extracting owner-local branches first, starting with the global-load emitter. |
| private `instruction_result_value_ref`, `instruction_result_prepared_value_id` | `dispatch_producers.cpp` | current-block join prepared query support | Keep with current-block join routing helpers; they decode instruction result identities for prepared facts. |
| private `prepare_current_block_join_parallel_copy_source_facts`, `prepared_query_current_block_join_parallel_copy_source`, `build_current_block_join_prepared_query_routing`, `current_block_join_prepared_query_incoming_expression`, `current_block_join_prepared_query_source` | `dispatch_producers.*` | current-block join publication | Natural destination is the current-block join/dispatch route owner; keep together because they share one prepared query fact bundle. |
| private `prepared_source_producer_for_value`, `prepared_same_block_select_producer`, public `find_prepared_same_block_select_producer` | `dispatch_producers.*` | still-shared prepared producer authority / select | Selector producer lookup is shared by select materialization; do not move into an unrelated owner without a select-owner packet. |
| private `prepared_select_chain_contains_direct_global_load`, public `select_chain_contains_direct_global_load` | `dispatch_producers.*` | globals-aware select producer query | Leave for later; likely either globals or select/materialization owner after the first global emitter move proves boundaries. |
| `producer_instruction_index` | `dispatch_producers.*` | dispatch orchestration / producer lookup | Thin wrapper over MIR query used by comparison and FP materialization hooks; can move with hook wiring, not first. |

## Suggested Next

Execute Step 2 with the first bounded relocation packet:

Move private `emit_prepared_global_load_to_register` out of
`dispatch_value_materialization.cpp` and into the globals owner
(`globals.cpp`/`globals.hpp`) as a globals-owned helper for prepared same-block
global load publication. Update `emit_value_publication_to_register` to call
the globals helper, reuse the existing globals relocation formatting where
possible, and leave the rest of value publication untouched.

## Watchouts

- Do not mix edge-copy relocation into this plan.
- Do not re-derive prepared publication authority locally.
- Do not claim progress through file-count reduction alone.
- Preserve diagnostics and machine-record behavior.
- First packet should not move `emit_value_publication_to_register` wholesale;
  extract only the globals-owned emitter and any declaration needed by the call
  site.
- If the globals helper needs prepared same-block producer context beyond the
  current access/load pair, stop and report the ownership ambiguity instead of
  widening into scalar materialization.

## Proof

No code proof run for this documentation-only Step 1 packet, and no
`test_after.log` was created.

Recommended proof for the first relocation packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The full backend regex is recommended for the first move because global-load
publication is reached through the shared value-publication hook used by memory,
comparison, FP, edge-copy, calls, and dispatch paths.
