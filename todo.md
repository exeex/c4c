Status: Active
Source Idea Path: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Update Encoder, Extractor, And Contract Tests

# Current Packet

## Just Finished

Step 3 implemented the EV64 `.insn.d` prefixed layout and updated the focused
contract tests.

Implementation:

- `src/backend/mir/riscv/codegen/object_emission.cpp` now emits `.insn.d` with
  fixed low bits `[6:0] = 0x3f`, destination/lhs/rhs in the RV32-like register
  positions, `major` in `[31:25]`, `operation` in `[39:32]`, accumulator as
  `rs4/msk` in `[44:40]`, reserved `[47:45] = 0`, and `dtype` in `[63:48]`.
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp` uses the same layout
  for the standalone c4c-as line encoder while preserving the existing
  seven-operand `.insn.d` text surface and validation.
- `src/apps/c4c-objdump.cpp` decodes only the new prefixed layout and rejects
  nonzero reserved `[14:12]` or `[47:45]` bits fail-closed.

Focused tests/docs:

- Updated `backend_riscv_object_emission_test` expected 64-bit words for both
  canonical source text and prepared inline-asm operand substitution.
- Updated `backend_cli_riscv64_vrm_insn_d_source_obj`,
  `backend_c4c_as_parse_suite`, `backend_c4c_objdump_extract_suite`, and
  `backend_rv64_roundtrip_contract` expected bytes.
- The canonical `.insn.d 10, 11, v6, v0, v2, v4, 3` bytes are now
  `3f0320140b040300` (`0x0003040b1420033f`).
- The prepared substituted `.insn.d 10, 11, v20, v4, v6, v8, 3` bytes are now
  `3f0a62140b080300` (`0x0003080b14620a3f`).
- The objdump unsupported fixture now corrupts reserved `[47:45]` bits and
  proves the new layout fails closed at `.text` offset `0x0`.

## Suggested Next

Execute Step 4: run the plan's final representative/acceptance proof for idea
385, including the LLVM one-instruction consumption check against freshly
emitted c4c EV64 bytes if the supervisor wants that separate from this focused
test subset.

## Watchouts

- Preserve the source idea's scope: EV64 `.insn.d` length-prefix layout only.
- Do not implement unrelated RV64 object-route lowering or upstream LLVM
  decoder-table support.
- Keep the existing seven-operand `.insn.d` surface stable; this plan moves
  fields in the binary encoding and does not add/remove operands.
- c4c-objdump intentionally does not decode the old `major`-in-low-bits layout.
- Keep parser validation and malformed operand rejection intact; Step 3 changed
  binary placement, not operand count or operand kinds.

## Proof

Command:

```bash
{
  cmake --build --preset default --target backend_riscv_object_emission_test c4c-as c4c-objdump c4cll
  ctest --test-dir build -R 'backend_riscv_object_emission|backend_c4c_as_parse_suite|backend_c4c_objdump_extract_suite|backend_rv64_roundtrip_contract|backend_cli_riscv64_vrm_insn_d_source_obj' --output-on-failure
} > test_after.log 2>&1
```

Result: passed, 5/5 focused tests green. Proof log: `test_after.log`.
