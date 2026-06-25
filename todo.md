Status: Active
Source Idea Path: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Minimal RV64 Assembly File Parser

# Current Packet

## Just Finished

Step 2 added the minimal `c4c-as` RV64 assembly file parser and CLI argument
handling.

- `c4c-as <input.s> -o <output.o>` now reads the input file and parses the
  initial supported file syntax: `.text`, `.globl <symbol>`, `<label>:`, blank
  lines, and `#` comments.
- Instruction lines are validated through the shared RV64 line parser instead
  of duplicating `.insn.d`, `li`, or `ret` operand parsing locally.
- The parser fails closed with diagnostics for unsupported directives,
  malformed labels, instruction lines outside `.text`, unsupported options,
  missing input, and missing `-o`.
- Object writing remains deliberately unimplemented; successful parsing reports
  the number of collected instruction lines and the requested output path.
- `c4c-as` links `c4c_backend` only when `ENABLE_C4C_BACKEND` is enabled.
- Added the focused `backend_c4c_as_parse_suite` CTest suite covering
  canonical valid RV64 input plus fail-closed syntax paths.

## Suggested Next

Execute Step 3 by connecting the parsed RV64 lines to the existing encoder and
RV64 relocatable ELF object writer for the canonical single-function `.text`
subset.

## Watchouts

- Do not start idea 351 before `c4c-as` can assemble the canonical subset.
- The parser currently accepts and records `.globl` symbols and labels but does
  not yet map them into object writer functions/fragments.
- Object writing should reuse `encode_rv64_asm_line(...)`,
  `build_rv64_text_object_module(...)`, and
  `write_rv64_relocatable_elf_object(...)`; avoid adding local instruction
  encoders in `c4c-as`.
- CTest did not select both tests with the grouped regex
  `^(backend_c4c_as_|backend_cli_riscv64_vrm_insn_d_source_obj)$`; use the
  equivalent `^backend_(c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)` proof
  shape for this packet family.

## Proof

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)'
```

Result: passed. Proof log: `test_after.log`.
