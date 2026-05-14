Status: Active
Source Idea Path: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Introduce Or Normalize Central Spelling Helpers

# Current Packet

## Just Finished

Completed Step 3: introduced a central supported printer mnemonic helper
surface for AArch64 machine-node/operator spelling without routing
`machine_printer.cpp` through it yet.

Implementation updates made:
- `codegen/records.hpp` now exposes `MachinePrinterMnemonicKind` plus helper
  functions that map supported machine opcodes, pseudos, and instruction
  records to printer mnemonic spelling.
- `codegen/records.cpp` centralizes the currently supported printer spellings:
  `b`, `cbnz`, `ldr`, `str`, `mov`, and `ret`.
- Unsupported or non-printable opcode/operator cases return the empty spelling
  through the new helper surface.
- `backend_aarch64_target_instruction_records_test.cpp` covers representative
  helper output for branch, conditional branch, load/store, immediate-return
  `mov`, `ret`, and an unsupported scalar opcode.

## Suggested Next

Execute Step 4: route the AArch64 printer and relevant diagnostics through the
shared spelling helpers while preserving existing printed assembly.

## Watchouts

- Keep idea 218's boundary intact: printed `.s` is terminal renderer output,
  not semantic backend input.
- Do not touch `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`.
- Do not touch `ideas/open/221_backend_test_tree_split_bir_mir_and_prune_legacy.md`.
- Do not expand instruction selection coverage just to add more operator names.
- Do not downgrade expectations or introduce testcase-shaped shortcuts.
- The live printer still has hard-coded mnemonics; Step 4 should consume the
  central helper without treating printer text as semantic input.
- External assembler and encoder headers still accept mnemonic strings for the
  external text path only; do not treat those as the internal shared operator
  source.
- The new helper intentionally returns an empty spelling for currently
  unsupported scalar opcodes and `CompareBranch`; do not expand instruction
  selection coverage just to make those spellings non-empty.

## Proof

Step 3 helper/test proof:

```bash
bash -lc 'set -o pipefail; cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_(target_instruction_records|machine_printer)$" >> test_after.log 2>&1'
```

The delegated build plus focused AArch64 target-instruction records and
machine-printer subset passed. Proof output is preserved in `test_after.log`.
