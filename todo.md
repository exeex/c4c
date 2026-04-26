Status: Active
Source Idea Path: ideas/open/119_bir_block_label_structured_identity_for_assembler.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Intern Labels During LIR-To-BIR Lowering

# Current Packet

## Just Finished

Step 3 lowering population completed in `src/backend/bir/lir_to_bir/module.cpp`
and `tests/backend/backend_lir_to_bir_notes_test.cpp`.

Lowering now interns known lowered BIR block labels once through
`bir::Module::names.block_labels` after each function lowers, assigns
`bir::Block::label_id`, and fills matching structured ids for scalar phi
incoming labels plus branch and conditional-branch terminator labels. Unknown
label references preserve the raw string and keep `kInvalidBlockLabel`.

Added a direct lowering test covering normal blocks, switch-generated compare
blocks, branch targets, scalar phi incoming labels, and an unresolved phi
incoming label without relying on printer text.

## Suggested Next

Next coherent packet: teach the next downstream consumer to prefer structured
block label ids where available while preserving raw-string fallbacks and
current dump/validation contracts.

## Watchouts

- Preserve existing BIR dump text, including block-header trailing colons.
- Keep raw label strings as fallbacks until a later cleanup idea removes them.
- Do not expand this plan into MIR target codegen migration.
- Same-spelled labels such as `entry` across functions will share an id under
  the module table, which matches the prepared backend spelling-table model.
- The Step 3 helper intentionally only resolves labels present in the lowered
  BIR function, so malformed/unresolved references are not interned as if they
  were known blocks.

## Proof

Passed. Proof log: `test_after.log`.

Command run exactly:
`( cmake --build build-backend --target c4cll backend_lir_to_bir_notes_test && ctest --test-dir build-backend/tests/backend -R '(backend_lir_to_bir_notes|backend_cli_dump_bir_(is_semantic|vla_goto_stackrestore_cfg|focus_block_entry_00204|focus_value_t1_00204))$' --output-on-failure ) > test_after.log 2>&1`

Result: `c4cll` and `backend_lir_to_bir_notes_test` built successfully and all
5 selected backend tests passed.
