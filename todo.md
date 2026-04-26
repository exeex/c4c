Status: Active
Source Idea Path: ideas/open/119_bir_block_label_structured_identity_for_assembler.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current BIR Label Authority

# Current Packet

## Just Finished

Step 1 inventory completed for current BIR label authority. Chosen owner:
add a small BIR module-owned name-table container, analogous to
`prepare::PreparedNameTables`, with `TextTable texts` and
`BlockLabelTable block_labels{&texts}` owned by `bir::Module`. Prefer a
wrapper over raw `Module` members so copy/move can reattach the semantic table
to the copied/moved text storage, matching the prepared backend pattern.

Raw BIR label surfaces found:
- `bir::Block::label`
- `bir::PhiIncoming::label`, used by both `bir::PhiInst::incomings` and
  `bir::PhiObservation::incomings`
- `bir::Terminator::target_label`
- `bir::Terminator::true_label`
- `bir::Terminator::false_label`
- `bir::MemoryAddress::BaseKind::Label` plus `MemoryAddress::base_name`

Current writers/readers:
- LIR-to-BIR writes block labels from LIR blocks and synthetic switch compare
  blocks in `src/backend/bir/lir_to_bir/module.cpp`.
- LIR-to-BIR writes phi incoming labels in `lower_block_phi_insts`.
- LIR-to-BIR writes branch/conditional/switch target labels in
  `lower_block_terminator`.
- No current LIR-to-BIR writer for `MemoryAddress::BaseKind::Label` was found;
  the raw surface exists in BIR and the printer handles it through
  `base_name`.
- `bir_printer.cpp` renders all current label surfaces from raw strings.
- `bir_validate.cpp` validates block, phi, branch, and cond-branch labels by
  raw string membership in each function.
- Focused BIR dump filtering in `backend.cpp` compares focus block names
  against `Block::label`; value focus can also match `MemoryAddress::base_name`.

Prepared-backend label-table touchpoints:
- `PreparedNameTables` already owns module-level `TextTable` and
  `BlockLabelTable`.
- `legalize.cpp` interns block labels and branch targets into
  `PreparedControlFlowBlock` / `PreparedBranchCondition`.
- `out_of_ssa.cpp` interns phi incoming and join/continuation labels into
  join-transfer and parallel-copy records, and sometimes converts ids back to
  strings for rewritten BIR phi incoming labels.
- `liveness.cpp`, `regalloc.cpp`, `prealloc.cpp`, stack-layout coordination,
  and prepared filtering resolve/find labels through `names.block_labels` and
  `prepared_block_label`.
- `prepared_printer.cpp` renders prepared label ids through
  `PreparedNameTables::block_labels`.

## Suggested Next

First code-changing packet: add the BIR module-owned name-table wrapper and
default-invalid `BlockLabelId` fields beside the existing raw string fields in
`bir.hpp` only: block label id, phi incoming label id, branch target id,
conditional true/false ids, and label-address base id. Do not change lowering,
validation, filtering, or printing behavior in this packet.

Focused proof command for that packet:
`cmake --build build --target c4cll backend_cli_dump_bir_is_semantic backend_cli_dump_bir_vla_goto_stackrestore_cfg backend_cli_dump_bir_focus_block_entry_00204 backend_cli_dump_bir_focus_value_t1_00204 && ctest --test-dir build -R 'backend_cli_dump_bir_(is_semantic|vla_goto_stackrestore_cfg|focus_block_entry_00204|focus_value_t1_00204)$' --output-on-failure`

## Watchouts

- Preserve existing BIR dump text, including block-header trailing colons.
- Keep raw label strings as fallbacks until a later cleanup idea removes them.
- Do not expand this plan into MIR target codegen migration.
- Module-owned label ids are spelling ids, not function-scoped CFG ownership;
  raw/function-local validation should remain authoritative until validation is
  deliberately taught to cross-check ids.
- Because `BlockLabelTable` stores a pointer to `TextTable`, do not put the two
  directly on `bir::Module` without reattachment-safe copy/move handling.
- Same-spelled labels such as `entry` across functions will share an id under
  the module table, which matches the prepared backend spelling-table model.

## Proof

No build required and no `test_after.log` created; this packet was
inventory-only by supervisor instruction. Inspection used `rg` and targeted
reads of `bir.hpp`, `bir_printer.cpp`, `bir_validate.cpp`,
`lir_to_bir/module.cpp`, `lir_to_bir/cfg.cpp`, `backend.cpp`,
`text_id_table.hpp`, and prepared backend label-table touchpoints.
