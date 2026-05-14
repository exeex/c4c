Status: Active
Source Idea Path: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Representative Drift Guards

# Current Packet

## Just Finished

Completed Step 5: added representative AArch64 mnemonic drift guards that keep
supported helper output and printed assembly spelling aligned.

Implementation updates made:
- `backend_aarch64_target_instruction_records_test.cpp` now directly proves
  the supported shared helper spellings for `str`, `ldr`, `cbnz`, `b`, `mov`,
  and `ret`.
- `backend_aarch64_machine_printer_test.cpp` now derives representative
  expected printer output from the shared helper spellings and compares that
  helper-derived output against canonical printed assembly for spill/reload,
  conditional branch plus fallthrough branch, store, immediate move, and
  return.
- No instruction-selection coverage was expanded and no expectations were
  downgraded.

## Suggested Next

Execute Step 6: perform the final contract and validation pass for idea 219,
confirming the acceptance criteria and reviewer reject signals before broader
supervisor-selected validation.

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
- The helper intentionally returns an empty spelling for currently
  unsupported scalar opcodes and `CompareBranch`; do not expand instruction
  selection coverage just to make those spellings non-empty.
- The printer still intentionally rejects loads without a structured
  destination operand; Step 4 did not add new printable instruction forms.
- Step 5 intentionally guards the currently supported printer subset only:
  `str`, `ldr`, `cbnz`, `b`, `mov`, and `ret`.

## Proof

Step 5 drift-guard proof:

```bash
bash -lc 'set -o pipefail; cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_(target_instruction_records|machine_printer)$" >> test_after.log 2>&1'
```

The delegated build plus focused AArch64 target-instruction records and
machine-printer subset passed. Proof output is preserved in `test_after.log`.
