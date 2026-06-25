Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Rewire Inline Asm Object Emission Through The Line Core

# Current Packet

## Just Finished

Step 4 rewired RV64 `.insn.d` inline asm object emission through the line core.

- `fragment_for_rv64_insn_d_inline_asm(...)` now calls
  `substitute_prepared_riscv_inline_asm_operands(...)`, parses the substituted
  canonical text with `parse_rv64_asm_line(...)`, and encodes it with
  `encode_rv64_asm_line(...)`.
- The object path now rejects non-`.insn.d` parsed lines in this fragment and
  preserves the existing fail-closed checks inherited from substitution.
- Existing source-level `.insn.d` object bytes and `.s` output stayed green.

## Suggested Next

Execute Step 5 by proving shared text/object compatibility more explicitly.

Owned files for the next packet should include:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

The next packet should add or tighten tests that take substituted inline asm
text and prove the same line core accepts it. Prefer a focused unit assertion
near the existing substitution/object tests, while keeping the source-level
object byte and `.s` route checks intact.

## Watchouts

- Do not start ideas 350 or 351 before idea 349 provides the shared line core.
- Do not add new testcase-shaped `.insn.d` object special cases.
- Keep inline asm constraints and VRM allocation out of the line parser.
- Legacy `.insn.d` carrier classifier helpers still exist for tests and
  comparison; do not route new object behavior back through prepared
  placeholder fields.
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

Use the same narrow proof command for the Step 5 compatibility packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_codegen_route_riscv64_vrm_insn_d_source_asm)$'
```
