Status: Active
Source Idea Path: ideas/open/119_bir_block_label_structured_identity_for_assembler.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Label Fields Beside Raw Strings

# Current Packet

## Just Finished

Step 2 storage addition completed in `src/backend/bir/bir.hpp`.
Added `bir::NameTables`, a module-owned `TextTable` plus `BlockLabelTable`
wrapper with reattachment-safe copy and move construction/assignment, and
attached it to `bir::Module`.

Added default-invalid `BlockLabelId` storage beside the existing raw string
label surfaces:
- `bir::Block::label_id`
- `bir::PhiIncoming::label_id`
- `bir::BranchTerminator::target_label_id`
- `bir::CondBranchTerminator::true_label_id` and `false_label_id`
- `bir::Terminator::target_label_id`, `true_label_id`, and `false_label_id`
- `bir::MemoryAddress::base_label_id`

No lowering, validation, filtering, printer, or test behavior was changed.
Existing raw strings remain authoritative for this packet.

## Suggested Next

Next coherent packet: populate the new structured BIR label ids at existing
raw-label write sites, while preserving current raw-string behavior and dump
output.

## Watchouts

- Module-owned label ids are spelling ids, not function-scoped CFG ownership;
  raw/function-local validation should remain authoritative until validation is
  deliberately taught to cross-check ids.
- Preserve existing BIR dump text, including block-header trailing colons.
- Keep raw label strings as fallbacks until a later cleanup idea removes them.
- Do not expand this plan into MIR target codegen migration.
- Same-spelled labels such as `entry` across functions will share an id under
  the module table, which matches the prepared backend spelling-table model.

## Proof

Passed. Proof log: `test_after.log`.

Command run exactly:
`( cmake --build build-backend --target c4cll && ctest --test-dir build-backend/tests/backend -R 'backend_cli_dump_bir_(is_semantic|vla_goto_stackrestore_cfg|focus_block_entry_00204|focus_value_t1_00204)$' --output-on-failure ) > test_after.log 2>&1`

Result: `c4cll` built successfully and all 4 selected backend CLI dump tests
passed.
