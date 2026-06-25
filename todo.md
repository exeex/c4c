Status: Active
Source Idea Path: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Existing c4c-as Placeholder And Object Writer Surfaces

# Current Packet

## Just Finished

Step 1 located the `c4c-as` placeholder and RV64 object writer surfaces.

- CLI placeholder: `src/apps/c4c-as.cpp` currently handles help/version and
  otherwise exits with `c4c-as: assembler input is not implemented yet`.
- Build wiring: top-level `CMakeLists.txt` defines `c4c-as` as a standalone
  executable, not yet linked to `c4c_backend`.
- RV64 line core: `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
  exposes `parse_rv64_asm_line(...)` and `encode_rv64_asm_line(...)`.
- Object writer APIs:
  `RiscvEncodedFragment`, `RiscvObjectFunction`,
  `build_rv64_text_object_module(...)`, and
  `write_rv64_relocatable_elf_object(...)` in
  `src/backend/mir/riscv/codegen/object_emission.hpp`.
- Existing source-route comparison test:
  `backend_cli_riscv64_vrm_insn_d_source_obj` checks the expected object bytes
  `0a0320080b0300001305000067800000`.
- No existing `c4c-as` assembly tests were found.

## Suggested Next

Execute Step 2 by adding the minimal file parser and CLI argument handling in
`src/apps/c4c-as.cpp`.

Owned files for the next packet should include:

- `src/apps/c4c-as.cpp`
- top-level `CMakeLists.txt` if `c4c-as` needs backend linkage
- `tests/backend/CMakeLists.txt` and/or a small backend CMake runner if adding
  CLI tests immediately
- `todo.md`

The packet should parse `c4c-as <input.s> -o <output.o>`, accept `.text`,
`.globl <symbol>`, `<label>:`, blank lines, comments, and collect instruction
lines without parsing operands locally. It may stop before object writing if it
lands focused negative/diagnostic tests, but it must not duplicate `.insn.d`,
`li`, or `ret` parsing.

## Watchouts

- Do not start idea 351 before `c4c-as` can assemble the canonical subset.
- Do not duplicate `.insn.d`, `li`, or `ret` operand parsing in the file parser.
- Unsupported directives or instructions must fail closed, not be ignored.
- If `c4c-as` starts using RV64 backend APIs, ensure the app target links
  `c4c_backend` only in backend-enabled builds or otherwise preserves
  non-backend builds deliberately.

## Proof

Discovery-only Step 1 update; no build proof required.

Suggested proof command once Step 2 adds `c4c-as` tests:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_c4c_as_|backend_cli_riscv64_vrm_insn_d_source_obj)$'
```
