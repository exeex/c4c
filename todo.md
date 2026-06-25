Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Existing RV64 Inline Asm Surfaces

# Current Packet

## Just Finished

Step 1 located the RV64 inline asm surfaces for idea 349.

- Substitution surface: `src/backend/mir/riscv/codegen/object_emission.cpp`
  exposes `substitute_prepared_riscv_inline_asm_operands(...)`; lower-level
  template substitution also exists in `src/backend/mir/riscv/codegen/inline_asm.cpp`.
- Current object direct-encoding surface:
  `fragment_for_rv64_insn_d_inline_asm(...)` in
  `src/backend/mir/riscv/codegen/object_emission.cpp` still classifies the
  prepared carrier through `classify_prepared_rv64_insn_d_inline_asm(...)` and
  encodes with `encode_rv64_ev_insn_d_inline_asm(...)`.
- Existing parser-like helpers are private to `object_emission.cpp`:
  `trim_ascii(...)`, `split_rv64_insn_fields(...)`,
  `parse_rv64_insn_u32(...)`, and `rv64_register_number(...)`.
- Source-level proof surfaces are
  `backend_cli_riscv64_vrm_insn_d_source_obj` for object bytes and
  `backend_codegen_route_riscv64_vrm_insn_d_source_asm` for canonical text.
- Focused unit surface is `tests/backend/mir/backend_riscv_object_emission_test.cpp`
  under CTest name `backend_riscv_object_emission`.

## Suggested Next

Execute Step 2 by extracting a target-local RV64 single-line parser surface
from the private `object_emission.cpp` helpers.

Owned files for the next packet should include:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- new target-local line-core files under `src/backend/mir/riscv/codegen/` if
  needed
- `src/backend/CMakeLists.txt` or `src/backend/mir/riscv/codegen` build wiring
  only if new translation units are added
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

The first code packet should keep behavior unchanged while making canonical
line parsing callable for `.insn.d 10, 11, v6, v0, v2, v4, 3`, `li a0, 0`,
and `ret`. It should not yet rewire the inline asm object path unless the
parser extraction is already complete and small.

## Watchouts

- Do not start ideas 350 or 351 before idea 349 provides the shared line core.
- Do not add new testcase-shaped `.insn.d` object special cases.
- Keep inline asm constraints and VRM allocation out of the line parser.
- The current `.insn.d` classifier rejects literal canonical fields because it
  reads prepared operand placeholders directly; Step 2 needs a real canonical
  line parser that accepts physical registers and immediates.
- `li` and `ret` encoding helpers already exist indirectly as
  `append_rv64_load_immediate(...)` plus the `jalr x0, 0(ra)` return encoding,
  but they are not exposed as a line encoder yet.
- Keep source-level object byte assertion
  `0a0320080b0300001305000067800000` intact.

## Proof

Discovery-only Step 1 update; no build proof required.

Narrow proof command for the first code packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_codegen_route_riscv64_vrm_insn_d_source_asm)$'
```
