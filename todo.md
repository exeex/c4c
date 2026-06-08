Status: Active
Source Idea Path: ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Wide-Value Helper Clusters

# Current Packet

## Just Finished

Step 1: Map Wide-Value Helper Clusters completed the initial read-only cluster
map for the two AArch64 wide-value owner files.

`src/backend/mir/aarch64/codegen/i128_ops.cpp` cluster map:

- Diagnostics/name helpers, printable register checks, and target print wrappers
  (`append_i128_pair_diagnostic`, `*_error_message`, `target_*`, kind/error name
  helpers): local-organization-only.
- i128 runtime helper policy validation (`complete_preserved_value_route`,
  `has_complete_live_preservation`, `has_complete_selected_call_ownership`,
  `has_complete_i128_helper_resource_policy`,
  `has_complete_i128_div_rem_abi_policy`,
  `validate_i128_div_rem_helper_marshaling_plan`): appears to consume prepared
  helper/resource/ABI facts, but uncertain pending `calls.cpp` overlap
  inspection because it revalidates preservation, selected-call ownership, and
  div/rem ABI shape locally.
- i128 runtime helper printing (`append_i128_helper_move_line`,
  `print_i128_runtime_helper`): aarch64-codegen-consumption for target `mov` and
  `bl` assembly from prepared marshal facts.
- carrier and transport record construction (`make_i128_lane_register_operand`,
  `make_prepared_i128_carrier_transport_record`,
  `make_prepared_i128_copy_transport_record`): mostly
  aarch64-codegen-consumption of prepared i128 carrier facts; memory-backed
  branches are uncertain pending `memory.cpp` overlap inspection.
- pair operation record construction (`make_i128_pair_carrier_lane_adapter`,
  `is_supported_i128_pair_opcode`, `i128_pair_*_from_binary_opcode`,
  `make_prepared_i128_pair_operation_record`): aarch64-codegen-consumption for
  AArch64 register-pair record shape and supported-opcode selection.
- shift record construction (`is_i128_shift_opcode`,
  `i128_shift_*_from_binary_opcode`, `is_supported_i128_shift_count`,
  `make_prepared_i128_shift_record`): aarch64-codegen-consumption; count
  storage uses prepared scalar/value-location facts, while lane semantics remain
  target machine-record spelling.
- compare record construction (`i128_compare_*_from_predicate`,
  `make_prepared_i128_compare_record`): aarch64-codegen-consumption of prepared
  i128 carriers and scalar result storage; uncertain only for overlap with
  printer/instruction record shape.
- machine instruction wrappers/status checks (`i128_*_selection_status`,
  `make_i128_*_instruction`, lower entry points): aarch64-codegen-consumption;
  validates prepared facts before building operands/defs/uses/clobbers and
  dispatching lowering.

`src/backend/mir/aarch64/codegen/f128.cpp` cluster map:

- Kind/error names, diagnostics, register spelling, Q/S/D/W/X display names,
  and target print wrappers: local-organization-only.
- f128 carrier facts (`prepared_f128_full_width_carrier_facts`,
  `prepared_f128_memory_backed_carrier_facts`, `make_f128_register_operand`):
  consumes prepared carrier facts; memory-backed carrier facts are uncertain
  pending `memory.cpp`/`calls.cpp` overlap inspection because the file rebuilds
  frame-slot memory operands for printable transport.
- printable address and memory-backed transport helpers
  (`f128_printable_memory_address`, `f128_memory_backed_carrier_memory`,
  `print_f128_transport`): aarch64-codegen-consumption for printable address
  assembly, but uncertain around memory-backed publication/authority and should
  inspect `memory.cpp`.
- helper resource/ABI/preservation checks (`has_complete_f128_*`,
  `f128_*_matches`, `f128_helper_policies_match_record`,
  `validate_f128_runtime_helper_source_policy`): appears to consume prepared
  helper policy, but uncertain pending `calls.cpp` overlap inspection because it
  deeply matches resource, ABI, clobber, preservation, selected-call ownership,
  carrier binding, scalar bridge, and cmp-result consumption facts.
- helper move and scalar bridge printing (`append_f128_helper_move_line`,
  `append_f128_scalar_move_line`, `validate_f128_cmp_scalar_result`,
  `print_f128_runtime_helper`): aarch64-codegen-consumption for target `mov`,
  `fmov`, `cmp`, `cset`, and `bl` assembly from prepared marshal facts.
- runtime helper record construction (`make_f128_abi_register_operand`,
  `make_f128_scalar_register_operand`,
  `make_f128_cmp_materialized_i1_register_operand`,
  `make_f128_helper_operand_record`, `make_f128_helper_scalar_record`,
  `make_prepared_f128_runtime_helper_boundary_record`):
  aarch64-codegen-consumption of prepared helper/carrier/scalar facts; uncertain
  around whether helper-family/result-ownership/ABI validation duplicates shared
  helper policy.
- transport and helper machine instruction wrappers/status checks
  (`validate_f128_transport_instruction`,
  `validate_f128_runtime_helper_boundary_instruction`,
  `make_f128_transport_instruction`,
  `make_f128_runtime_helper_boundary_instruction`,
  `lower_f128_runtime_helper_instruction`): aarch64-codegen-consumption, with
  uncertain overlap against `instruction.cpp`/`machine_printer.cpp` for
  printable record shape and status diagnostics.

## Suggested Next

Execute Step 2 from `plan.md`: inspect i128 runtime helper ownership, div/rem
ABI policy, preserved value checks, and selected-call ownership against
`src/backend/mir/aarch64/codegen/calls.cpp`.

## Watchouts

- This route is analysis-only; do not edit implementation files, tests, or
  build metadata.
- Do not treat `i128_ops.cpp` or `f128.cpp` size as evidence of a boundary gap.
- Do not move AArch64 register spelling, Q-register spelling, lane/shift
  opcode spelling, or helper call assembly into shared BIR/prealloc code.
- Follow-up ideas must be concrete: owner boundary, filenames, proof route, and
  reject signals.
- Step 2 should prioritize whether i128 helper resource/ABI, preservation, and
  selected-call validation is duplicate policy or just completeness validation
  for prepared facts.
- Later overlap checks should inspect f128 memory-backed carrier printable
  address handling against `memory.cpp`, and f128 helper policy matching against
  `calls.cpp`.

## Proof

Proof: analysis-only, no build/test run.
