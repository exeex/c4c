Status: Active
Source Idea Path: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Assemble Instructions Through The RV64 Line Core

# Current Packet

## Just Finished

Step 3 connected collected `c4c-as` RV64 instruction lines to the shared line
core parser and encoder.

- Instruction lines are stored as parsed `Rv64AsmLine` values, so `c4c-as`
  calls `parse_rv64_asm_line(...)` during file parsing and does not duplicate
  `.insn.d`, `li`, or `ret` operand parsing locally.
- Successful parsing now calls `encode_rv64_asm_line(...)` for each parsed
  instruction and appends the bytes into one text fragment.
- Line-core parse and encode failures fail closed with diagnostics instead of
  silently skipping instructions.
- Object writing remains deliberately unimplemented for Step 3; successful
  assembly reports the text fragment hex on stdout and does not write `-o`.
- The focused `backend_c4c_as_parse_suite` now proves the canonical
  three-instruction input observes text bytes
  `0a0320080b0300001305000067800000`.

## Suggested Next

Execute Step 4 by building the minimal RV64 relocatable ELF object from the
supported `.globl main` / `main:` model using the existing object writer APIs.

## Watchouts

- Do not start idea 351 before `c4c-as` writes the canonical relocatable object.
- The parser accepts and records `.globl` symbols and labels but still does not
  map them into object writer functions/fragments.
- Step 4 should reuse `build_rv64_text_object_module(...)` and
  `write_rv64_relocatable_elf_object(...)`; avoid adding local instruction
  encoders in `c4c-as`.
- Step 3 intentionally leaves `-o` unwritten; the parse suite checks that this
  pre-object behavior remains true until Step 4 owns object emission.

## Proof

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)'
```

Result: passed. Proof log: `test_after.log`.
