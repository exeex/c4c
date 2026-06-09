Status: Active
Source Idea Path: ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Map Public Call Sites

# Current Packet

## Just Finished

Completed `plan.md` Step 2: Map Public Call Sites.

Public declaration call-site map:

- `dispatch.hpp`
  - `InstructionDispatchResult`: only the return/local result type for `dispatch_prepared_block` in `dispatch.hpp:22` and `dispatch.cpp:465`, with local construction at `dispatch.cpp:469`; no independent external use.
  - `make_block_lowering_context`: external orchestration hook called by `lower_prepared_functions` in `traversal.cpp:98`; clang caller query confirmed `traversal.cpp:64`.
  - `dispatch_prepared_block`: external orchestration hook called by `lower_prepared_functions` in `traversal.cpp:103`; clang caller query confirmed `traversal.cpp:64`.
- `dispatch_edge_copies.hpp`
  - `EdgeProducerContext`: type is used only inside `dispatch_edge_copies.cpp` helper flow (`:94`, `:142`, `:148`, `:192`, `:198`, `:236`, `:384`, `:406`) and the public `emit_edge_load_local_to_register` signature; no external type use.
  - `edge_value_publication_may_read_register_index`: called only by nearby edge-copy implementation for recursive and hazard checks in `dispatch_edge_copies.cpp` (`:327`, `:341`, `:348`, `:358`, `:365`, `:767`, `:775`, `:970`, `:978`, `:1177`); no caller outside `dispatch_edge_copies.cpp`.
  - `emit_edge_load_local_to_register`: public wrapper has no direct callers outside its declaration/definition; implementation uses `emit_edge_load_local_to_register_impl`.
  - `emit_edge_value_publication_to_register`: called only by nearby edge-copy code at `dispatch_edge_copies.cpp:1112`; recursive/internal calls use `emit_edge_value_publication_to_register_impl`.
  - `should_emit_block_entry_edge_copy_move`: dispatch-family hook called by `dispatch.cpp:504` and by edge-copy filtering at `dispatch_edge_copies.cpp:1150`.
  - `lower_predecessor_select_parallel_copy_sources`: dispatch-family hook called by the main dispatcher at `dispatch.cpp:985`.
- `dispatch_lookup.hpp`
  - `make_named_prepared_result_register`: external codegen query used by `globals.cpp:929`, `globals.cpp:1147`, `memory_store_retargeting.cpp:127`, `comparison.cpp:1979`, `alu.cpp:3838`, `memory.cpp:2515`, `memory.cpp:4535`, and `calls.cpp` (`:6150`, `:6349`, `:6363`, `:6378`, `:6430`).
  - `emitted_scalar_value_available`: external call-lowering query used only by `calls.cpp:6342` and `calls.cpp:6421`.
  - `is_scalar_call_argument_producer_opcode`: no direct callers beyond declaration/definition.
  - `find_same_block_scalar_producer`: no direct callers beyond declaration/definition.
  - `has_same_block_load_local_producer`: no direct callers beyond declaration/definition.
- `dispatch_producers.hpp`
  - `SameBlockSelectProducer`: public alias used only by `dispatch_producers.cpp` and as the return type of `find_prepared_same_block_select_producer`; no external alias use beyond the function contract.
  - `find_prepared_same_block_select_producer`: external select materialization query used by `select_materialization.cpp:127`.
  - `prepared_publication_source_producer_for_value`: hook-table/exported producer query used in `dispatch.cpp:424-425`, `dispatch_publication.cpp:329-330`, and through `comparison.hpp` hooks by `comparison.cpp:1677`; also recursively used inside `dispatch_producers.cpp:133`.
  - `prepared_source_producer_instruction`: hook-table/exported producer query used in `dispatch.cpp:426-427`, `dispatch_publication.cpp:331-332`, and through `comparison.hpp` hooks by `comparison.cpp:1681`.
  - `select_chain_contains_direct_global_load`: no direct callers beyond declaration/definition.
  - `producer_instruction_index`: external producer-index query used by `fp_value_materialization.cpp` (`:213`, `:280`, `:339`, `:406`) and exported through comparison hook tables at `dispatch.cpp:437` and `dispatch_publication.cpp:342`; `calls.cpp` uses the shared `mir::producer_instruction_index` directly, not this wrapper.
  - `prepared_query_current_block_join_parallel_copy_source`: nearby value-materialization query used by `dispatch_value_materialization.cpp:169`.
  - `CurrentBlockJoinPreparedQueryRouting`: type is constructed and consumed only in `dispatch_producers.cpp` and by the public current-block routing functions; no external type use except via those signatures.
  - `build_current_block_join_prepared_query_routing`: dispatch-family hook called by `dispatch.cpp:512`.
  - `current_block_join_prepared_query_incoming_expression`: dispatch-family hook called by `dispatch.cpp:720`.
  - `current_block_join_prepared_query_source`: dispatch-family hook called by `dispatch.cpp:829`.
  - `block_entry_move_clobbers_current_join_publication`: dispatch-family hazard hook called by `dispatch.cpp:563` and `dispatch_edge_copies.cpp:1150`.
  - `prepared_value_home_reads_register_index`: nearby register-read helper called by `dispatch_producers.cpp:490`, `dispatch_producers.cpp:496`, `dispatch_edge_copies.cpp:319`, and `dispatch_edge_copies.cpp:378`.
  - `value_publication_may_read_register_index`: external hazard query used by `alu.cpp:3994-3996` and nearby value-materialization hazard checks at `dispatch_value_materialization.cpp` (`:601`, `:603`, `:671`, `:673`); recursive internal calls remain in `dispatch_producers.cpp:504-520`.
- `dispatch_publication.hpp`
  - `integer_bit_width`: external scalar/cast utility used by `cast_ops.cpp:1434-1435` and `cast_ops.cpp:1554-1555`; nearby uses in `dispatch_edge_copies.cpp:659-660` and `dispatch_value_materialization.cpp:327-328`, `:360-361`.
  - `power_of_two_shift`: external ALU utility used by `alu.cpp:48`, `alu.cpp:54`, and `alu.cpp:3913`; nearby edge-copy use at `dispatch_edge_copies.cpp:944`.
  - `relocation_operand`: dispatch-publication declaration is used by `memory.cpp:3702`, `memory.cpp:4636`, `fp_value_materialization.cpp:131`, `fp_value_materialization.cpp:394`, and `calls.cpp:6976`; same-name local helpers in `globals.cpp`, `machine_printer.cpp`, and `alu.cpp` are separate definitions and not this declaration.
  - `scalar_view_for_type`: broad external scalar utility used by `prepared_value_home_materialization.cpp`, `fp_value_materialization.cpp`, `globals.cpp`, `select_materialization.cpp`, `variadic.cpp`, `alu.cpp`, `cast_ops.cpp`, `memory.cpp`, `calls.cpp`, and through comparison hooks; nearby uses remain in `dispatch.cpp`, `dispatch_edge_copies.cpp`, `dispatch_publication.cpp`, and `dispatch_value_materialization.cpp`.
  - `gp_register_name`: broad external scalar utility used by `prepared_value_home_materialization.cpp`, `globals.cpp`, `select_materialization.cpp`, `variadic.cpp`, `alu.cpp`, `cast_ops.cpp`, and `memory.cpp`; nearby uses remain in edge-copy and value-materialization code.
  - `scalar_integer_width_bits`: external utility used by `fp_value_materialization.cpp:410` and `memory.cpp:3776`; local use in `dispatch_publication.cpp:135`.
  - `scalar_gp_register_view`: dispatch-publication declaration is used by `memory.cpp:3749`; similarly named static owner helpers in `calls.cpp` are separate.
  - `scalar_fp_register_view`: dispatch-publication register-reference overload is used by `fp_value_materialization.cpp`, `select_materialization.cpp:335`, and `dispatch_value_materialization.cpp:441-442`; same-name type-only or local overloads in `float_ops.cpp`, `memory.cpp`, and `calls.cpp` are separate declarations.
  - `immediate_integer_bits`: dispatch-publication declaration is used by `fp_value_materialization.cpp:422` and `memory.cpp:3788`; same-name static owner helper in `calls.cpp` is separate.
  - `is_byval_formal_value_name`: external memory query used by `memory.cpp:3285`.
  - `prepared_value_home_for_value`: broad external prepared-home query used by `fp_value_materialization.cpp`, `globals.cpp`, `comparison.cpp` through hooks, `variadic.cpp`, `cast_ops.cpp`, `memory.cpp`, and `calls.cpp`; nearby uses remain in `dispatch.cpp`, `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, and `dispatch_value_materialization.cpp`.
  - `value_has_current_block_entry_publication`: hook-table/exported query used in `dispatch.cpp:439-440`, `dispatch_publication.cpp:344-345`, `comparison.cpp:2241`, and `dispatch_value_materialization.cpp:191`; nearby producer use at `dispatch_producers.cpp:489`.
  - `current_block_entry_publication_register`: external current-block publication query used by `alu.cpp:78`, `comparison.cpp:2183`, and through comparison hooks at `comparison.cpp:1725`, `comparison.cpp:2085`; nearby value-materialization use at `dispatch_value_materialization.cpp:153` and hook export at `dispatch.cpp:428-429`, `dispatch_publication.cpp:333-334`.
  - `record_current_block_entry_publication_registers`: dispatch-family hook called only by `dispatch.cpp:492`.
  - `lower_missing_conditional_branch_condition_publication`: dispatch-family wrapper called only by `dispatch.cpp:1028`; it delegates to comparison lowering in `dispatch_publication.cpp:359`.
- `dispatch_value_materialization.hpp`
  - `emit_value_publication_to_register`: broad external materialization hook used by `fp_value_materialization.cpp`, `select_materialization.cpp`, `comparison.cpp` through hooks, `alu.cpp`, `cast_ops.cpp`, `memory.cpp`, and `calls.cpp`; nearby recursive/internal uses remain in `dispatch_edge_copies.cpp`, `dispatch_publication.cpp`, and `dispatch_value_materialization.cpp`.

External orchestration/public hooks evidenced by callers outside the dispatch
family:

- Top-level lowering: `make_block_lowering_context`, `dispatch_prepared_block`.
- Shared codegen queries/utilities: `make_named_prepared_result_register`,
  `emitted_scalar_value_available`, `find_prepared_same_block_select_producer`,
  `producer_instruction_index`, `value_publication_may_read_register_index`,
  most `dispatch_publication.hpp` scalar/publication utilities, and
  `emit_value_publication_to_register`.
- Comparison branch-fusion hook-table exports:
  `prepared_publication_source_producer_for_value`,
  `prepared_source_producer_instruction`, `scalar_view_for_type`,
  `emit_value_publication_to_register`,
  `current_block_entry_publication_register`,
  `producer_instruction_index`, `prepared_value_home_for_value`, and
  `value_has_current_block_entry_publication`.

Declarations currently evidenced as only nearby dispatch-family organization
artifacts or unused public surface:

- Edge-copy-only/dispatch-only: `EdgeProducerContext`,
  `edge_value_publication_may_read_register_index`,
  `emit_edge_load_local_to_register`,
  `emit_edge_value_publication_to_register`,
  `should_emit_block_entry_edge_copy_move`,
  `lower_predecessor_select_parallel_copy_sources`.
- Producer/current-block dispatch-only:
  `SameBlockSelectProducer`,
  `prepared_query_current_block_join_parallel_copy_source`,
  `CurrentBlockJoinPreparedQueryRouting`,
  `build_current_block_join_prepared_query_routing`,
  `current_block_join_prepared_query_incoming_expression`,
  `current_block_join_prepared_query_source`,
  `block_entry_move_clobbers_current_join_publication`,
  `prepared_value_home_reads_register_index`.
- No direct callers found beyond declaration/definition:
  `is_scalar_call_argument_producer_opcode`,
  `find_same_block_scalar_producer`,
  `has_same_block_load_local_producer`,
  `select_chain_contains_direct_global_load`.

## Suggested Next

Execute Step 3 from `plan.md`: classify each dispatch-family `.cpp` and `.hpp`
as `keep-public-hook`, `fold-back`, `shared-contract-gap`,
`target-local-emission`, or `local-organization-only` using the Step 1
inventory and Step 2 call-site map.

## Watchouts

- This is an analysis-only plan; do not edit implementation files.
- Do not treat file-count reduction as sufficient justification for fold-back.
- Generic names require overload/definition care during classification:
  `relocation_operand`, `scalar_gp_register_view`,
  `scalar_fp_register_view`, and `immediate_integer_bits` have same-name local
  or static helpers in other files.
- Public functions with no direct callers should be treated as fold-back or
  header-surface candidates only after Step 3 checks whether they are retained
  for a near-term contract reason; Step 2 found call-site evidence only.

## Proof

Analysis-only packet. Ran symbol/call-site inspection and captured proof output
in `test_after.log`:

- `c4c-clang-tool-ccdb function-callers ... traversal.cpp dispatch_prepared_block ...`
- `c4c-clang-tool-ccdb function-callers ... traversal.cpp make_block_lowering_context ...`
- `c4c-clang-tool-ccdb function-callers ... memory.cpp relocation_operand ...`
- `c4c-clang-tool-ccdb function-callers ... fp_value_materialization.cpp scalar_fp_register_view ...`
- `c4c-clang-tool-ccdb function-callers ... dispatch.cpp lower_missing_conditional_branch_condition_publication ...`
- `c4c-clang-tool-ccdb function-callers ... comparison.cpp emit_value_publication_to_register ...`
- grouped `rg -n "\\b<symbol>\\b" src/backend/mir/aarch64/codegen -g '*.cpp' -g '*.hpp'` passes covering every public function/type declaration from Step 1

No build/test run required because no implementation code was edited.
