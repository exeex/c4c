Status: Active
Source Idea Path: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Build And Write The Relocatable RV64 Object

# Current Packet

## Just Finished

Step 4 builds and writes the minimal RV64 relocatable ELF object for the
supported one-function `.text` subset.

- `c4c-as` now requires exactly one `.globl` symbol and exactly one text label
  for object emission, and it preserves the matching `.globl main` / `main:`
  boundary as the emitted function symbol.
- Encoded instruction bytes are wrapped in one `RiscvEncodedFragment`, passed
  through `build_rv64_text_object_module(...)`, then serialized with
  `write_rv64_relocatable_elf_object(...)`.
- The generated ELF image bytes are written to the requested `-o` path.
- The focused `backend_c4c_as_parse_suite` now proves the object is written and
  its `.text` section contains
  `0a0320080b0300001305000067800000`.

## Suggested Next

Execute Step 5 by comparing the `c4c-as` object route against the corresponding
source-level inline asm object route and adding the planned unsupported syntax
coverage if the supervisor keeps this idea active.

## Watchouts

- The current object subset is intentionally one global function with a
  matching label; broader label layout, multiple functions, relocations, and
  extra sections remain non-goals for this plan.
- The parse suite now includes an inline CMake ELF `.text` extractor. If this
  grows beyond the focused c4c-as route, move the helper into a reusable test
  utility instead of expanding this script ad hoc.

## Proof

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)' | tee test_after.log
```

Result: passed. Proof log: `test_after.log`.
