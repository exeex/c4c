Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove Shared Text/Object Compatibility

# Current Packet

## Just Finished

Step 5 proved substituted inline asm text is accepted by the same line core.

- Added a focused unit assertion that the prepared `.insn.d` carrier
  substitutes to `.insn.d 10, 11, v20, v4, v6, v8, 3`.
- The test parses that substituted text with `parse_rv64_asm_line(...)` and
  encodes it with `encode_rv64_asm_line(...)`.
- The encoded substituted prepared line matches the existing EV64 object word
  `0x0000030b10620a0a`.
- Source-level object byte and `.s` route checks stayed green.

## Suggested Next

Execute Step 6 by running broader validation and deciding whether idea 349 is
ready for lifecycle closure.

Owned files for the next packet should include:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

The next packet should run the broader RV64/backend validation subset chosen by
the supervisor, inspect the final diff for testcase-overfit, and then call the
plan owner if the source idea appears complete.

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

Suggested broader validation command for Step 6:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```
