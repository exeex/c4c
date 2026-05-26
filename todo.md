Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Calls Helper Ownership

# Current Packet

## Just Finished

Step 1 - Inventory Calls Helper Ownership completed for the AArch64 calls
fold-back helper files without implementation edits.

Inventory summary:

- `calls_byval_aggregates.cpp`
  - Defines `is_aarch64_byval_register_lane_move` and
    `make_byval_register_lane_prepared_source`.
  - Both are declared in `calls.hpp`.
  - External call sites are only in `calls_moves.cpp`:
    `is_aarch64_byval_register_lane_move` at lines 1362, 1748, 1825, 1898,
    1962, 2025, and 2170; `make_byval_register_lane_prepared_source` at
    lines 1648, 1773, 1868, 2067, and 2187.
  - Includes: owns `calls.hpp` and `memory.hpp`; no outside file includes a
    byval-specific header.
  - Ownership recommendation: fold this file into `calls_moves.cpp` first,
    make both helpers namespace-local, and remove their declarations from
    `calls.hpp`.

- `calls_common.cpp`
  - Defines public call/shared operand helpers declared in `calls.hpp`:
    `outgoing_stack_argument_bytes`,
    `outgoing_stack_argument_base_register`,
    `va_start_overflow_area_stack_offset`,
    `complete_full_width_f128_carrier`,
    `complete_f128_constant_carrier`,
    `find_prepared_f128_carrier_in_module`,
    `scalar_fp_view_from_register_name`,
    `scalar_view_from_register_name`,
    `scalar_size_from_register_view`,
    `register_name_with_expected_view`,
    `make_register_operand_from_prepared_authority`,
    `make_f128_q_register_operand_from_carrier`,
    `make_scalar_call_argument_immediate`,
    `scalar_integer_register_view`,
    `scalar_integer_register_view_from_size`, and
    `scalar_integer_type_from_size`.
  - Defines namespace-local helpers:
    `align_to`, `incoming_stack_argument_size_bytes`,
    `incoming_stack_argument_alignment_bytes`,
    `entry_param_uses_incoming_stack`, `named_incoming_stack_bytes`,
    `function_has_call`, `fixed_frame_adjustment_bytes`, and
    `register_class_from_bank`.
  - External call sites:
    `calls.cpp` uses `outgoing_stack_argument_bytes` and
    `va_start_overflow_area_stack_offset`; `calls_moves.cpp` uses nearly all
    register/view/immediate/F128 helpers plus
    `outgoing_stack_argument_base_register`; `calls_dispatch_bridge.cpp` uses
    `scalar_integer_register_view_from_size`.
  - Non-helper field-name matches exist in `instruction.hpp` and
    `machine_printer.cpp` for `outgoing_stack_argument_bytes`; these are not
    calls to the helper.
  - Includes: owns `calls.hpp` and `dispatch_diagnostics.hpp`; outside
    includes of `calls.hpp` are `calls.cpp`, `calls_moves.cpp`,
    `calls_byval_aggregates.cpp`, `calls_dispatch_bridge.cpp`, `dispatch.cpp`,
    `f128.cpp`, `globals.cpp`, and `i128_ops.cpp`.
  - Ownership recommendation: keep these externally visible until narrower
    owner headers exist. Likely final split is `calls.cpp`-local stack/variadic
    sizing, `calls_moves.cpp`-local operand adapters, and bridge-local integer
    view use only if still needed.

- `calls_dispatch_bridge.hpp/.cpp`
  - Header declares the dispatch-facing calls owner API:
    `lower_scalar_call_argument_producers`, `lower_call_instruction`,
    `materialize_call_boundary_source_to_destination`,
    `retarget_call_boundary_source_to_emitted_scalar`,
    `record_call_boundary_destination`,
    `record_call_boundary_source_in_destination`,
    `call_boundary_move_reloads_prepared_stack_source`,
    `source_register_conflicts_with_materialized_address`,
    `materialize_indirect_call_callee_to_prepared_register`,
    `record_call_result_source_register`,
    `retarget_fpr_call_result_store_value_to_emitted_scalar`,
    `materialize_missing_frame_slot_call_arguments`, and
    `publish_stack_preserved_call_values`.
  - `.cpp` also defines bridge-local helpers including
    `make_dispatch_bridge_machine_instruction`,
    `materialize_local_aggregate_address_lines`,
    `materialize_local_aggregate_address_call_argument`,
    `find_prepared_call_argument_plan`,
    `find_prepared_frame_slot_call_argument_move`,
    `materialize_scalar_call_argument_value`,
    `find_bir_value_for_prepared_name`,
    `source_value_is_materialized_address`,
    local load/store effective access helpers,
    indirect callee materialization scratch helpers, and
    `emit_indirect_callee_value_to_register_with_csel`.
  - External call sites are all in `dispatch.cpp`: bridge functions are called
    across the call routing loop at lines 488, 548, 555, 556, 592, 602, 617,
    622, 627, 655, 665, 671, 676, 680, and 855.
  - Includes: `dispatch.cpp` includes `calls_dispatch_bridge.hpp`; the bridge
    `.cpp` includes `dispatch.hpp`, `dispatch_edge_copies.hpp`,
    `calls_dispatch_bridge.hpp`, `calls.hpp`, and adjacent dispatch/call
    helpers.
  - Ownership recommendation: this header is the narrow API that must remain
    externally visible to `dispatch.cpp` unless/until dispatch routing is moved
    into a calls owner. Do not fold this first; it is the current integration
    boundary.

- `calls_moves.cpp`
  - Defines public helpers declared in `calls.hpp`:
    `make_selected_call_argument_source`, `find_move_bundle`,
    `call_boundary_move_reloads_materialized_address`,
    `order_before_call_moves_for_source_preservation`,
    `lower_before_call_moves`, `lower_after_call_moves`,
    `lower_before_return_moves`, and `lower_value_moves`.
  - Defines many namespace-local helpers grouped around call-boundary effects,
    selected source conversion, stack/local aggregate sources, before-call
    lowering, immediate ABI bindings, after-call lowering, callee-saved
    preservation publication, move ordering, and generic prepared move
    lowering.
  - External call sites: `dispatch.cpp` calls `lower_value_moves`,
    `lower_before_call_moves`, `order_before_call_moves_for_source_preservation`,
    `call_boundary_move_reloads_materialized_address`,
    `lower_after_call_moves`, and `lower_before_return_moves`; the bridge calls
    `make_selected_call_argument_source` and `find_move_bundle`.
  - Includes: owns `calls.hpp`, `constant_materialization.hpp`,
    `dispatch_diagnostics.hpp`, `dispatch.hpp`, `dispatch_lookup.hpp`,
    `dispatch_publication_common.hpp`, `f128.hpp`, `float_ops.hpp`, and
    `memory.hpp`.
  - Ownership recommendation: keep the dispatch-called movement API externally
    visible through either `calls_dispatch_bridge.hpp` or a narrowed calls
    owner header. `make_selected_call_argument_source` and `find_move_bundle`
    must remain visible to the bridge until the bridge and moves are folded
    together.

Build metadata:

- `src/backend/CMakeLists.txt` lists all four `.cpp` files in `C4C_BACKEND_SOURCES`
  at lines 17-20.
- `build/compile_commands.json` has entries for all four translation units at
  lines 622-642.
- `build/build.ninja` has object rules for all four files at lines 1098-1124
  and links all four objects into `src/backend/c4c_backend.a` at line 2050.

Helpers that must remain externally visible through a narrow calls-owner API
for the next fold-back stage:

- Dispatch-facing route API currently in `calls_dispatch_bridge.hpp`: all
  declared functions there are consumed by `dispatch.cpp`.
- Calls-moves API consumed by `dispatch.cpp`: `lower_value_moves`,
  `lower_before_call_moves`, `order_before_call_moves_for_source_preservation`,
  `call_boundary_move_reloads_materialized_address`, `lower_after_call_moves`,
  and `lower_before_return_moves`.
- Bridge-to-moves API: `make_selected_call_argument_source` and
  `find_move_bundle`.
- Call construction API still consumed by `calls.cpp`:
  `outgoing_stack_argument_bytes` and `va_start_overflow_area_stack_offset`.

## Suggested Next

First fold-back packet: fold `calls_byval_aggregates.cpp` into
`calls_moves.cpp`, make `is_aarch64_byval_register_lane_move` and
`make_byval_register_lane_prepared_source` namespace-local in `calls_moves.cpp`,
remove their declarations from `calls.hpp`, and remove
`calls_byval_aggregates.cpp` from `src/backend/CMakeLists.txt`.

## Watchouts

Keep the first fold-back mechanical: no call ABI semantics, prepared call
planning, byval source selection, preservation publication, return
publication, diagnostics, or test expectation changes. `calls_common.cpp`
contains helpers used across `calls.cpp`, `calls_moves.cpp`, and
`calls_dispatch_bridge.cpp`; folding it before narrowing the API would be much
riskier than the byval helper fold-back.

## Proof

No build required by delegated packet. Inventory used `rg` plus
`c4c-clang-tool-ccdb function-signatures` for the four `.cpp` helper files and
`c4c-clang-tool function-signatures` for `calls_dispatch_bridge.hpp`; no
`test_before.log` or `test_after.log` was created or modified.
