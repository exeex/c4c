Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add The RV64 Line Encoder

# Current Packet

## Just Finished

Step 3 added the reusable RV64 single-line encoder surface.

- Added `encode_rv64_asm_line(...)` for parsed `Rv64AsmLine` values.
- The encoder emits the existing EV64 `.insn.d` layout, simple signed-12-bit
  `li`, and `ret`.
- Added a focused unit check proving parsed canonical lines:
  `.insn.d 10, 11, v6, v0, v2, v4, 3`, `li a0, 0`, and `ret` encode to
  `0a0320080b0300001305000067800000`.
- Source-level object and asm routes still pass, but inline asm object emission
  has not been rewired yet.

## Suggested Next

Execute Step 4 by rewiring RV64 inline asm object emission through substituted
canonical text and the new line core.

Owned files for the next packet should include:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

The next packet should change `fragment_for_rv64_insn_d_inline_asm(...)` so it
calls `substitute_prepared_riscv_inline_asm_operands(...)`, parses the
substituted canonical line with `parse_rv64_asm_line(...)`, and encodes it with
`encode_rv64_asm_line(...)`. Keep the existing fail-closed carrier checks for
unsupported named operands, template modifiers, clobbers, and missing facts.

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

Use the same narrow proof command for the Step 4 rewire packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_codegen_route_riscv64_vrm_insn_d_source_asm)$'
```
