Status: Active
Source Idea Path: ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build Dispatch Family Inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 1: Build Dispatch Family Inventory.

Audited AArch64 dispatch-family files:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`

Header public declaration surface:

- `dispatch.hpp`: `InstructionDispatchResult`; `make_block_lowering_context`; `dispatch_prepared_block`.
- `dispatch_edge_copies.hpp`: `EdgeProducerContext`; `edge_value_publication_may_read_register_index`; `emit_edge_load_local_to_register`; `emit_edge_value_publication_to_register`; `should_emit_block_entry_edge_copy_move`; `lower_predecessor_select_parallel_copy_sources`.
- `dispatch_lookup.hpp`: `make_named_prepared_result_register`; `emitted_scalar_value_available`; `is_scalar_call_argument_producer_opcode`; `find_same_block_scalar_producer`; `has_same_block_load_local_producer`.
- `dispatch_producers.hpp`: `SameBlockSelectProducer` alias; `CurrentBlockJoinPreparedQueryRouting`; `find_prepared_same_block_select_producer`; `prepared_publication_source_producer_for_value`; `prepared_source_producer_instruction`; `select_chain_contains_direct_global_load`; `producer_instruction_index`; `prepared_query_current_block_join_parallel_copy_source`; `build_current_block_join_prepared_query_routing`; `current_block_join_prepared_query_incoming_expression`; `current_block_join_prepared_query_source`; `block_entry_move_clobbers_current_join_publication`; `prepared_value_home_reads_register_index`; `value_publication_may_read_register_index`.
- `dispatch_publication.hpp`: `integer_bit_width`; `power_of_two_shift`; `relocation_operand`; `scalar_view_for_type`; `gp_register_name`; `scalar_integer_width_bits`; `scalar_gp_register_view`; `scalar_fp_register_view`; `immediate_integer_bits`; `is_byval_formal_value_name`; `prepared_value_home_for_value`; `value_has_current_block_entry_publication`; `current_block_entry_publication_register`; `record_current_block_entry_publication_registers`; `lower_missing_conditional_branch_condition_publication`.
- `dispatch_value_materialization.hpp`: `emit_value_publication_to_register`.

Implementation responsibility map:

- `dispatch.cpp`: owns block context construction and the main prepared-block instruction/terminator dispatcher. File-local helpers cover instruction classification, diagnostics, BIR/prepared block lookup, stack-home/before-return publication checks, address-materialization fallback paths, and branch-fusion hook assembly.
- `dispatch_edge_copies.cpp`: owns predecessor/edge publication materialization and edge-copy hazard checks. File-local helpers resolve prepared edge producers, source producer contexts, memory/register sources, select-chain state, and recursive edge value publication; public entry points provide edge value/load emission, register-read queries, block-entry edge-copy filtering, and predecessor select parallel-copy lowering.
- `dispatch_lookup.cpp`: owns small prepared-result and scalar-producer lookup queries. File-local helpers resolve prepared names and prepared call-argument source materialization; public entry points expose prepared result register lookup, scalar availability, call-argument producer opcode checks, same-block scalar producer lookup, and same-block load-local producer checks.
- `dispatch_producers.cpp`: owns prepared producer/routing queries and publication register-read analysis. File-local helpers resolve prepared value IDs, instruction results, current-block join source facts, same-block select/publication producers, and BIR block lookup; public entry points expose prepared producer lookup, producer instruction/index lookup, current-block join query routing, block-entry clobber detection, prepared-home register-read checks, and recursive value-publication register-read checks.
- `dispatch_publication.cpp`: owns publication/register view utility queries and current-block publication hooks. File-local helpers resolve prepared names, collect current-block entry publications, find current-block publication homes, and assemble publication branch-fusion hooks; public entry points expose integer width/shift/relocation helpers, scalar register view helpers, immediate bit extraction, byval formal checks, prepared value home lookup, current-block publication register tracking, and missing conditional-branch condition publication lowering.
- `dispatch_value_materialization.cpp`: owns generic value publication materialization into a target register. File-local helpers resolve prepared names, prepared same-block scalar producers, scalar select-chain materialization, and integer constants; the public entry point handles prepared homes, current-block publications, load/global/select/binary/cast/immediate materialization, recursive operands, scratch/target hazard checks, and reload-current-memory-load behavior.

Build and metadata touchpoints:

- `src/backend/CMakeLists.txt` explicitly enumerates all six AArch64 dispatch-family `.cpp` files in `C4C_BACKEND_SOURCES`: `dispatch.cpp`, `dispatch_edge_copies.cpp`, `dispatch_lookup.cpp`, `dispatch_producers.cpp`, `dispatch_publication.cpp`, and `dispatch_value_materialization.cpp`.
- No dispatch-family headers are separately enumerated in the CMake source list.
- Backend test CMake metadata references `dispatch_value_materialization` only as labels for selected AArch64 probes at `tests/backend/CMakeLists.txt:99`, `:183`, `:194`, `:214`, and `:225`; `tests/backend/cmake/run_backend_aarch64_selected_indirect_call_case.cmake:41` contains an assembly label string `dispatch_table:`.

## Suggested Next

Execute Step 2 from `plan.md`: build the direct call-site map for every public
dispatch-family declaration and separate external orchestration hooks from
nearby dispatch-family organization artifacts.

## Watchouts

- This is an analysis-only plan; do not edit implementation files.
- Do not create follow-up ideas until concrete ownership or contract gaps are
  evidenced by the inventory and call-site map.
- Do not treat file-count reduction as sufficient justification for fold-back.
- `dispatch_publication.hpp` exposes generic helper names that overlap with
  other AArch64 files (`relocation_operand`, scalar register view helpers);
  Step 2 should distinguish exact overload/signature use, not only bare-name
  text matches.
- Standalone header clang queries produced usable declaration records but also
  reported a missing transitive `ast.hpp` include under minimal `-I` flags; the
  implementation-file symbol inventory used the compile database successfully.

## Proof

Analysis-only packet. Ran delegated inventory/build-metadata proof commands and
captured output in `test_after.log`:

- `find src/backend/mir/aarch64/codegen -maxdepth 1 \( -name 'dispatch*.cpp' -o -name 'dispatch*.hpp' \) -print | sort`
- `rg -n "dispatch(_|\\.)|dispatch_edge|dispatch_lookup|dispatch_producers|dispatch_publication|dispatch_value_materialization" CMakeLists.txt src tests -g 'CMakeLists.txt' -g '*.cmake'`

No build/test run required because no implementation code was edited.
