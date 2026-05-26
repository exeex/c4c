Status: Active
Source Idea Path: ideas/open/10_calls_printing_and_effects_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Call Printing And Effect Ownership

# Current Packet

## Just Finished

Completed Step 1 - Audit Call Printing And Effect Ownership. This was an
audit-only packet; no implementation files were changed.

Inspected files/symbols:

- `src/backend/mir/aarch64/codegen/calls.hpp`: call-family declarations exposed
  from `calls_common`, `calls_moves`, and call machine-node constructors.
- `src/backend/mir/aarch64/codegen/calls.cpp`: `make_call_instruction`,
  `make_call_boundary_move_instruction`,
  `make_call_boundary_abi_binding_instruction`,
  `lower_prepared_call_instruction`, `find_prepared_call_plan`,
  `require_prepared_call_plan`, and call selection-status helpers.
- `src/backend/mir/aarch64/codegen/calls_common.cpp`:
  `register_display_name`, `occupied_register_views`,
  `prepared_clobber_expected_view`, stack/call ABI sizing helpers, and
  diagnostics helpers.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`: call-boundary semantic
  lowering entrypoints `lower_before_call_moves`, `lower_after_call_moves`,
  `lower_before_return_moves`, `lower_value_moves`,
  `make_selected_call_argument_source`, plus local effect endpoint helpers
  `effect_value_id`, `effect_value_name`,
  `make_callee_saved_preservation_home_population`, and
  `make_callee_saved_preservation_home_republication_instruction`.
- `src/backend/mir/aarch64/codegen/machine_printer.hpp` and
  `machine_printer.cpp`: `MachineInstructionPrinter`,
  `print_machine_instruction_line_payloads`, `print_call`,
  `print_call_boundary_move`, `print_call_boundary_abi_binding`, and
  call-boundary print-only helpers including
  `print_aggregate_register_lane_publication_lines`,
  `print_call_boundary_frame_slot_load_lines`,
  `materialize_call_boundary_frame_slot_address_lines`,
  `call_boundary_load_width_bytes`,
  `call_boundary_frame_slot_direct_offset_is_encodable`,
  `call_boundary_address_scratch_register`,
  `f128_call_boundary_vector_register_name`,
  `scalar_call_boundary_integer_width_bits`,
  `scalar_call_boundary_integer_immediate_bits`, and
  `is_single_move_wide_immediate`.
- `src/backend/mir/aarch64/codegen/instruction.hpp` and `instruction.cpp`:
  printer mnemonic APIs `machine_instruction_primary_printer_mnemonic*`,
  `machine_instruction_auxiliary_printer_mnemonic*`, enum spelling helpers
  `machine_effect_resource_kind_name` and `machine_side_effect_kind_name`,
  generic effect builders `machine_effect_from_operand`,
  `machine_prepared_value_def`, `machine_effects_from_operands`, and
  call-prepared effect builders `effects_from_prepared_call_clobbers` and
  `effects_from_prepared_call_preserved_values`.
- `src/backend/mir/printer.hpp` and `printer.cpp`: shared MIR traversal and
  print-result ownership (`print_machine_function`, `print_machine_module`,
  `TargetInstructionPrinter`, `MachinePrintContext`).
- Tests located by CTest: `backend_aarch64_machine_printer`,
  `backend_aarch64_target_instruction_records`,
  `backend_call_boundary_effect_plan`,
  `backend_x86_call_boundary_effect_ordering`,
  `backend_aarch64_call_boundary_owner`, `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_branch_control_lowering`,
  `backend_aarch64_memory_operand_records`, and
  `backend_aarch64_prepared_memory_operand_records`.

Classification and migration map:

- Semantic call lowering, keep in call owners:
  `lower_prepared_call_instruction`, `lower_before_call_moves`,
  `lower_after_call_moves`, `lower_before_return_moves`, `lower_value_moves`,
  `make_selected_call_argument_source`, `find_move_bundle`,
  `make_callee_saved_preservation_home_population`,
  `make_callee_saved_preservation_home_republication_instruction`,
  `find_prepared_call_plan`, `require_prepared_call_plan`,
  `make_call_instruction`, `make_call_boundary_move_instruction`, and
  `make_call_boundary_abi_binding_instruction`. These choose or construct
  machine-node facts and effects; they are not printer-only code.
- Target-specific printer hooks, keep under an AArch64 printer owner:
  `MachineInstructionPrinter`, `print_machine_instruction_line_payloads`,
  `print_call`, `print_call_boundary_move`,
  `print_call_boundary_abi_binding`, and the call-boundary formatting helpers
  in `machine_printer.cpp`. They spell AArch64 assembly and diagnostics from
  already-selected machine nodes.
- Generic machine-node printer owner:
  shared traversal/result code in `src/backend/mir/printer.*` already belongs
  in shared MIR. The AArch64 line-payload dispatcher is target-specific but
  printer-owned; it should not be pulled into call lowering.
- Shared effect spelling/construction to migrate first:
  `machine_effect_resource_kind_name`, `machine_side_effect_kind_name`,
  `machine_effect_from_operand`, `machine_prepared_value_def`,
  `machine_effects_from_operands`,
  `effects_from_prepared_call_clobbers`, and
  `effects_from_prepared_call_preserved_values` should move out of the broad
  instruction/call surface into a shared AArch64 machine-effect owner such as
  `src/backend/mir/aarch64/codegen/effects.{hpp,cpp}`. The call-prepared
  clobber/preservation converters are call-aware effect construction, not
  semantic call lowering, and are shared by `calls.cpp`, `f128.cpp`, and
  `i128_ops.cpp`.
- Target register display helpers exposed through the call header:
  `register_display_name`, `occupied_register_views`, and
  `prepared_clobber_expected_view` are not semantic call lowering. They should
  move with the effect/operand owner or a target operand utility owner, then
  stop being exported from the call-family section of `calls.hpp`.
- Printer mnemonic APIs to move later:
  `machine_instruction_primary_printer_mnemonic*`,
  `machine_instruction_auxiliary_printer_mnemonic*`,
  `machine_opcode_printer_mnemonic_kind`, and
  `machine_pseudo_printer_mnemonic_kind` are target-printer spelling hooks.
  They can move with machine-printer cleanup after the effect helper migration.

Call-lowering dependencies on printer-only helpers:

- No direct dependency from `calls.cpp`, `calls_moves.cpp`, or `calls.hpp` to
  `MachineInstructionPrinter`, `print_call`,
  `print_call_boundary_move`, or `print_machine_instruction_line_payloads` was
  found.
- The real dependency to break first is call lowering depending on
  effect/display helpers from the broad instruction surface:
  `calls.cpp` calls `machine_effect_from_operand`, `machine_prepared_value_def`,
  `effects_from_prepared_call_clobbers`, and
  `effects_from_prepared_call_preserved_values`; `calls_common.cpp` exposes
  register display helpers through `calls.hpp`.
  These are not printer-only line emitters, but they are effect/target-spelling
  ownership that should not live behind the calls header.

Focused proof map for later packets:

- Step 2 effect migration:
  `cmake --build build -j && ctest --test-dir build -R '^(backend_call_boundary_effect_plan|backend_x86_call_boundary_effect_ordering|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' --output-on-failure`
- Step 3 machine-node printer migration:
  `cmake --build build -j && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_records)$' --output-on-failure`
- Step 4 retiring stale call printing surfaces:
  `cmake --build build -j && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_call_boundary_effect_plan)$' --output-on-failure`

## Suggested Next

Delegate Step 2 as a bounded effect-owner packet: create an AArch64 machine
effect owner (`effects.hpp`/`effects.cpp` or equivalent), move
`machine_effect_from_operand`, `machine_prepared_value_def`,
`machine_effects_from_operands`, `effects_from_prepared_call_clobbers`,
`effects_from_prepared_call_preserved_values`,
`machine_effect_resource_kind_name`, `machine_side_effect_kind_name`,
`register_display_name`, `occupied_register_views`, and
`prepared_clobber_expected_view` there, update includes/build metadata, and
leave call lowering behavior and printer output unchanged.

## Watchouts

- Step 2 should not touch `machine_printer.cpp` yet except for includes if the
  moved target display helpers are consumed there.
- Do not move `make_call_instruction`,
  `make_call_boundary_move_instruction`, or
  `make_call_boundary_abi_binding_instruction` in Step 2; they construct
  selected machine nodes and should stay with call emission until a separate
  machine-node-constructor owner is explicitly planned.
- Do not weaken tests or remove call preservation/clobber effect publication.
  `ScopedPreparedCallPreserveEffectPublication` is semantic emission control
  used by the assembler path, not a printer spelling helper.
- There is an unrelated untracked file under `review/` in the working tree;
  leave it untouched for this packet.

## Proof

Audit-only packet. No build required and no test log was produced. Verification
was by static inspection using `rg`, focused file reads,
`ctest --test-dir build -N`, and `c4c-clang-tool-ccdb function-signatures` on
`machine_printer.cpp`, `calls_common.cpp`, `calls.cpp`, and `calls_moves.cpp`.
