Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Instruction And Printer Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for the AArch64 instruction/printer surfaces.

- Record-kind name tables in `instruction.*`: `operand_kind_name`,
  `record_surface_kind_name`, `register_operand_role_name`,
  `immediate_kind_name`, `instruction_family_name`, `machine_opcode_name`,
  `machine_pseudo_kind_name`, `machine_node_selection_status_name`,
  `frame_instruction_kind_name`, `scalar_alu_operation_kind_name`,
  `scalar_unary_operation_kind_name`, `scalar_cast_operation_kind_name`,
  `branch_condition_form_name`, `branch_compare_candidate_kind_name`,
  `address_materialization_kind_name`; keep duplicated/public while callers use
  them for diagnostics, record dumps, and printer headers.
- Opcode mnemonic tables in `instruction.*`: `MachinePrinterMnemonicKind`,
  `machine_printer_mnemonic_kind_name`,
  `machine_opcode_printer_mnemonic_kind`,
  `machine_pseudo_printer_mnemonic_kind`,
  `machine_instruction_primary_printer_mnemonic_kind`,
  `machine_instruction_primary_printer_mnemonic`,
  `machine_instruction_auxiliary_printer_mnemonic_kind`,
  `machine_instruction_auxiliary_printer_mnemonic`; table-drive locally in the
  printer instead of publishing printer spelling through `instruction.hpp`.
- Local mnemonic helpers already in `machine_printer.cpp`:
  `scalar_memory_mnemonic`, `stack_publication_store_mnemonic`,
  `stack_source_load_mnemonic`, atomic load/store/exclusive/RMW mnemonic
  helpers, plus aggregate lane load use of
  `aggregate_register_lane_load_mnemonic`; keep local or fold into a new
  printer-local table helper as each family is contracted.
- Prepared-record error name tables in `instruction.*`:
  `prepared_branch_record_error_name`,
  `prepared_scalar_alu_record_error_name`,
  `prepared_scalar_cast_record_error_name`,
  `prepared_scalar_fp_unary_intrinsic_record_error_name`,
  `prepared_address_materialization_record_error_name`; keep duplicated/public
  for preparation diagnostics until there is a separate diagnostics-table
  policy. Header declares additional prepared error name APIs without matching
  definitions in this surface; do not widen Step 1 into that cleanup.
- Status builders in `instruction.cpp`: `branch_selection_status`,
  `scalar_selection_status`, `address_materialization_selection_status`,
  `spill_reload_selection_status`, `frame_selection_status`; keep in
  instruction construction for now because they produce
  `MachineNodeStatusRecord` from prepared record facts rather than printer
  spelling.
- Printer validation helpers in `machine_printer.cpp`: `bad_header`,
  `required_primary_mnemonic`, `required_auxiliary_mnemonic`,
  `validate_selected_machine_node`, `print_machine_instruction_line_payloads`,
  plus memory/address helpers such as `direct_stack_offset_is_encodable`,
  `frame_slot_direct_offset_is_encodable`, `same_gp_register_index`,
  `printable_memory_address`, and scratch selectors; fold into existing local
  helpers or table-drive locally, but do not weaken selected-node validation.

## Suggested Next

First bounded implementation packet: move the generic printer mnemonic-kind
spelling surface out of the public instruction API and make it printer-local.
Owned files: `src/backend/mir/aarch64/codegen/instruction.hpp`,
`src/backend/mir/aarch64/codegen/instruction.cpp`,
`src/backend/mir/aarch64/codegen/machine_printer.cpp`, and `todo.md`.
Focused proof command: `cmake --build build --target c4c && ctest --test-dir build -R aarch64 --output-on-failure`.

## Watchouts

Do not remove `machine_opcode_name`, `instruction_family_name`,
`record_surface_kind_name`, or `machine_node_selection_status_name`; `bad_header`
and selected-node diagnostics still depend on those public record names. The
first implementation packet should preserve byte-width special cases for memory,
atomic, stack-source, and aggregate-lane mnemonics rather than forcing them
through the generic opcode table. Header signature probing reported a missing
include path for `mir/printer.hpp`, but compile-db AST queries for both `.cpp`
targets succeeded.

## Proof

No code proof required; inventory-only packet. Exploratory commands used
`c4c-clang-tool-ccdb list-symbols` on `instruction.cpp` and
`machine_printer.cpp`, plus targeted `rg`/`sed` reads. No root-level proof log
was written.
