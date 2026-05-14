Status: Active
Source Idea Path: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Printer And Diagnostics Through Shared Spelling

# Current Packet

## Just Finished

Completed Step 4: routed the AArch64 machine printer through the shared
printer mnemonic helper surface while preserving existing printed assembly.

Implementation updates made:
- `codegen/machine_printer.cpp` now obtains supported printed mnemonics from
  the shared helper surface for spill/reload, branch, store, immediate-return
  move, and return output.
- Hard-coded supported printer spelling literals were removed from the printer
  body where practical; missing helper spellings fail closed with diagnostics.
- `backend_aarch64_machine_printer_test.cpp` still proves representative output
  and now checks helper/printer mnemonic alignment for `str`, `ldr`, `cbnz`,
  `b`, `mov`, and `ret`.

## Suggested Next

Execute Step 5: route remaining diagnostics or documentation references that
name printer/operator spelling through the shared vocabulary, without expanding
instruction selection coverage.

## Watchouts

- Keep idea 218's boundary intact: printed `.s` is terminal renderer output,
  not semantic backend input.
- Do not touch `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`.
- Do not touch `ideas/open/221_backend_test_tree_split_bir_mir_and_prune_legacy.md`.
- Do not expand instruction selection coverage just to add more operator names.
- Do not downgrade expectations or introduce testcase-shaped shortcuts.
- External assembler and encoder headers still accept mnemonic strings for the
  external text path only; do not treat those as the internal shared operator
  source.
- The new helper intentionally returns an empty spelling for currently
  unsupported scalar opcodes and `CompareBranch`; do not expand instruction
  selection coverage just to make those spellings non-empty.
- The printer still intentionally rejects loads without a structured
  destination operand; Step 4 did not add new printable instruction forms.

## Proof

Step 4 printer-routing proof:

```bash
bash -lc 'set -o pipefail; cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_(target_instruction_records|machine_printer)$" >> test_after.log 2>&1'
```

The delegated build plus focused AArch64 target-instruction records and
machine-printer subset passed. Proof output is preserved in `test_after.log`.
