Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add The RV64 Line Instruction Type And Parser

# Current Packet

## Just Finished

Step 2 added the reusable RV64 single-line parser surface.

- Added `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp` and `.cpp`
  with `Rv64AsmLine`, `Rv64InsnDLine`, `Rv64LiLine`, `Rv64RetLine`, physical
  GPR/vector register parsing, bounded immediates, and
  `parse_rv64_asm_line(...)`.
- Wired `rv64_line_assembler.cpp` into `c4c_backend`.
- Added focused parser tests in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` for canonical
  `.insn.d 10, 11, v6, v0, v2, v4, 3`, `li a0, 0`, and `ret`, plus malformed
  register, field-count, and immediate rejection cases.
- No object emission behavior was rewired in this packet.

## Suggested Next

Execute Step 3 by adding an encoder for parsed `Rv64AsmLine` values.

Owned files for the next packet should include:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

The encoder should reuse or match the existing RV64 object helper encodings for
`.insn.d`, simple `li`, and `ret`, and should prove the canonical three-line
sequence encodes to `0a0320080b0300001305000067800000`. Keep the inline asm
object path unchanged until the parser/encoder is complete.

## Watchouts

- Do not start ideas 350 or 351 before idea 349 provides the shared line core.
- Do not add new testcase-shaped `.insn.d` object special cases.
- Keep inline asm constraints and VRM allocation out of the line parser.
- The current `.insn.d` object path still classifies prepared carrier
  placeholders directly; Step 4 must replace that with substituted canonical
  text parsed by the line core.
- `li` and `ret` encoding helpers already exist indirectly as
  `append_rv64_load_immediate(...)` plus the `jalr x0, 0(ra)` return encoding,
  but they are not exposed as a line encoder yet.
- Keep source-level object byte assertion
  `0a0320080b0300001305000067800000` intact.

## Proof

Proof passed and was captured in `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_codegen_route_riscv64_vrm_insn_d_source_asm)$'
```

Result: 3/3 tests passed.

Use the same narrow proof command for the Step 3 encoder packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_codegen_route_riscv64_vrm_insn_d_source_asm)$'
```
