Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Argument-Source Helper Ownership

# Current Packet

## Just Finished

Step 1 audited `calls_argument_sources.cpp` helper ownership and direct caller
groups.

Public helper/declaration inventory from `calls.hpp` that is implemented in
`calls_argument_sources.cpp`:

- F128 source adapters: `complete_full_width_f128_carrier`,
  `complete_f128_constant_carrier`, `find_prepared_f128_carrier_in_module`,
  `make_f128_q_register_operand_from_carrier`.
- Register/view/immediate adapters:
  `scalar_fp_view_from_register_name`, `scalar_view_from_register_name`,
  `scalar_size_from_register_view`, `register_name_with_expected_view`,
  `make_register_operand_from_prepared_authority`,
  `make_scalar_call_argument_immediate`, `scalar_integer_register_view`,
  `scalar_integer_register_view_from_size`, `scalar_integer_type_from_size`.
- Memory source/destination builders: `find_frame_slot_by_id`,
  `make_sret_memory_return_address_source`,
  `make_frame_slot_call_argument_source`,
  `make_frame_slot_call_argument_address_source`,
  `make_local_frame_address_call_argument_source`,
  `make_stack_call_argument_destination`,
  `make_aggregate_call_argument_source`,
  `make_memory_return_storage`.
- Call/preservation adapters: `make_indirect_callee_register`,
  `make_prior_preserved_call_argument_source`.

Direct caller groups:

- `calls_moves.cpp` / `lower_before_call_move` owns most call-boundary source
  consumption: frame-slot values, frame-slot addresses, local frame-address
  materializations, aggregate/byval stack destinations, aggregate address
  sources, prior preservation, F128 carrier checks, and register operand
  construction.
- `calls_moves.cpp` / `lower_before_call_immediate_binding` uses scalar
  immediate and scalar view helpers.
- `calls_moves.cpp` / `lower_after_call_move`,
  `make_callee_saved_preservation_home_republication_instruction`,
  `make_callee_saved_preservation_home_population`,
  `lower_before_return_moves`, and `lower_value_moves` use the register/view
  adapters outside call-argument source selection.
- `calls_dispatch_bridge.cpp` /
  `materialize_missing_frame_slot_call_arguments` consumes only frame-slot
  value/address source helpers for missing scalar call-argument materialization.
- `calls.cpp` / `lower_prepared_call_instruction` consumes
  `make_indirect_callee_register` and `make_memory_return_storage`.
- `calls_byval_aggregates.cpp` / `collect_byval_register_lane_stores` and
  `cast_ops.cpp` / `prepared_frame_load_offset` share `find_frame_slot_by_id`.

Ownership classification:

- Semantic source choice still lives in
  `make_frame_slot_call_argument_source`,
  `make_frame_slot_call_argument_address_source`, and
  `make_local_frame_address_call_argument_source` when explicit
  `source_selection` is absent or incomplete. These helpers scan prepared
  addressing/source-access records and, for legacy compatibility, stack object
  names such as local aggregate first lanes.
- Explicit prepared-fact consumers are
  `make_selected_frame_slot_source`,
  `make_selected_local_frame_address_source`, and
  `make_prior_preserved_call_argument_source`; they should stay as adapters
  from selected facts to AArch64 operands, not choose alternate sources.
- Target operand construction/adapters are
  `make_register_operand_from_prepared_authority`,
  `make_f128_q_register_operand_from_carrier`,
  `make_scalar_call_argument_immediate`, scalar view/type helpers,
  `make_stack_call_argument_destination`,
  `make_aggregate_call_argument_source`,
  `make_indirect_callee_register`, and `make_memory_return_storage`.
- Diagnostics are centralized through `append_call_diagnostic` calls inside
  adapter helpers; no new diagnostic ownership should move into semantic source
  selection.
- Shared lookup utility `find_frame_slot_by_id` is not call-source-specific; it
  already serves byval aggregate and cast paths and should not be retired as
  part of argument-source helper retirement unless replaced with an equivalent
  shared lookup.

## Suggested Next

Step 2 should extract or narrow the target-only operand adapters first:
separate register/view/immediate/F128 construction from the helpers that still
choose frame-slot/local-address sources, then leave the three semantic
source-choice helpers in place until prepared records provide complete selected
facts for their absent-selection fallbacks.

## Watchouts

- Keep semantic source-choice authority in prepared call facts, not AArch64
  helper reshuffles.
- Do not edit implementation files, `ideas/closed/`, `review/`, or test logs
  as part of lifecycle activation.
- Treat expectation weakening, duplicate prepared/AArch64 source selection, and
  helper-only moves as route failures.
- Do not fold `find_frame_slot_by_id` into call-specific code; it is a shared
  lookup used by byval aggregate and cast lowering.
- Do not retire `make_frame_slot_call_argument_address_source`,
  `make_frame_slot_call_argument_source`, or
  `make_local_frame_address_call_argument_source` until the missing prepared
  facts for absent-selection local aggregate/address paths are available.

## Proof

Audit-only packet; no build proof required and no test logs were modified.

Tool commands used:

- `c4c-clang-tool-ccdb list-symbols
  /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_argument_sources.cpp
  build/compile_commands.json`
- `c4c-clang-tool function-signatures
  src/backend/mir/aarch64/codegen/calls.hpp -- --std=c++20
  -I/workspaces/c4c/src` returned declarations before an include-path error;
  header declaration lines were then confirmed by direct `sed`.
- `c4c-clang-tool-ccdb function-callers` for the public helper surface across
  `calls_moves.cpp`, `calls_dispatch_bridge.cpp`, `calls.cpp`,
  `calls_byval_aggregates.cpp`, and `cast_ops.cpp`.
- `c4c-clang-tool-ccdb function-callees` for
  `make_frame_slot_call_argument_source`,
  `make_frame_slot_call_argument_address_source`,
  `make_local_frame_address_call_argument_source`,
  `make_prior_preserved_call_argument_source`, and
  `make_register_operand_from_prepared_authority`.
