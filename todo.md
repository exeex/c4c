Status: Active
Source Idea Path: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Document The Naming Tiers And Alias Ownership

# Current Packet

## Just Finished

Completed Step 2: documented AArch64 naming tiers and alias ownership in the
contract surfaces without changing implementation behavior.

Contract updates made:
- `MACHINE_INSTRUCTION_NODE_CONTRACT.md` now separates stream item kind names,
  natural operator kind names, printer mnemonic names, and encoder canonical
  form names.
- `MACHINE_INSTRUCTION_NODE_CONTRACT.md` states aliases such as `Mov`, `Cmp`,
  and `Tst` are valid at the MIR / structured-asm layer when they improve
  readability and preserve typed semantics.
- `codegen/records.md` records that natural operator vocabulary is semantic
  backend input, printer mnemonic vocabulary is terminal `.s` output, and
  encoder canonical form vocabulary belongs to assembler/encoder
  canonicalization.
- `assembler/encoder/mod.hpp` clarifies that the encoder layer owns later alias
  canonicalization into concrete encoder canonical forms and must not use
  `machine_printer.cpp` printed `.s` as semantic backend input.

## Suggested Next

Execute Step 3: introduce or normalize central spelling helpers for the
supported stream item and operator names, keeping behavior changes limited to
shared spelling surfaces.

## Watchouts

- Keep idea 218's boundary intact: printed `.s` is terminal renderer output,
  not semantic backend input.
- Do not touch `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`.
- Do not touch `ideas/open/221_backend_test_tree_split_bir_mir_and_prune_legacy.md`.
- Do not expand instruction selection coverage just to add more operator names.
- Do not downgrade expectations or introduce testcase-shaped shortcuts.
- The live printer has hard-coded mnemonics; Step 3 should centralize spelling
  without treating printer text as semantic input.
- External assembler and encoder headers still accept mnemonic strings for the
  external text path only; do not treat those as the internal shared operator
  source.

## Proof

Step 2 contract wording proof:

```bash
bash -lc 'set -o pipefail; rg -n "stream item kind|natural operator kind|printer mnemonic|encoder canonical|Mov|Cmp|Tst|alias|canonicalization|printed \.s|semantic backend input" src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md src/backend/mir/aarch64/codegen/records.md src/backend/mir/aarch64/assembler/encoder/mod.hpp todo.md > test_after.log 2>&1; test -s test_after.log'
```

This docs/header contract packet changed no implementation behavior. The
delegated text-search proof is sufficient for Step 2 wording coverage and
writes `test_after.log`.
