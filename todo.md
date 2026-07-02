# Current Packet

Status: Active
Source Idea Path: ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Encoder And Byte Helper Surfaces

## Just Finished

Completed Step 1 - Map Encoder And Byte Helper Surfaces for
`ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md`.

Evidence used:

- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/rv64_line_assembler.cpp build/compile_commands.json`
- Targeted reads of `object_emission.cpp`, `object_emission.hpp`,
  `rv64_line_assembler.cpp`, and `rv64_line_assembler.hpp`.

Safe helper set:

- Pure word encoding: duplicated anonymous `encode_u_type`, `encode_i_type`,
  `encode_s_type`, `encode_r_type`, `encode_b_type`, and `encode_j_type`.
  These only mask/shift scalar opcode/register/immediate fields and do not
  reference prepared BIR, object modules, labels, fixups, parser state, or
  fragments.
- Pure little-endian byte append: duplicated anonymous `append_le32` and
  `append_le64`. These only append bytes to `std::vector<std::uint8_t>` and
  do not inspect object-route or text-parser state.

Selected Step 2 boundary:

- Destination owner: a neutral low-level RV64 encoding surface in the
  `rv64_line_assembler.*` ownership area, exposed with object-independent
  names such as `rv64_encode_*` and `rv64_append_le*`. The API must take only
  integer fields and byte vectors; it must not mention `RiscvEncodedFragment`,
  `Rv64AsmLine`, prepared BIR, labels, fixups, object modules, or parser
  records.
- Preserve wrappers in `object_emission.cpp` for the first implementation
  packet when they keep object-route call sites stable. The wrappers should
  delegate to the shared low-level helpers and preserve current byte order and
  immediate masking exactly.

Dependency classification:

- Pure word encoding: `encode_u_type`, `encode_i_type`, `encode_s_type`,
  `encode_r_type`, `encode_b_type`, `encode_j_type`.
- Pure LE byte append: `append_le32`, `append_le64`.
- Object-fragment-specific: `append_fragment`, `append_rv64_fragment`, and all
  helpers that construct or merge `RiscvEncodedFragment` records.
- Text-parser-specific: `trim_ascii`, `starts_with_token`, `split_fields`,
  parse field/register/immediate helpers, `parse_rv64_asm_line`,
  `encode_rv64_asm_line`, and `rv64_asm_line_size_bytes` because they depend
  on `Rv64AsmLine` variants, parser spelling, or line-size policy.
- Fixup/label-aware: direct call, PC-relative address, block-label branch/JAL,
  AUIPC/LO12, object module assembly, relocation mapping, local-label binding,
  and data-object relocation helpers.

Unsafe for Step 2:

- Do not move `append_fragment` or `append_rv64_fragment`; they adjust label
  and fixup offsets and are structured-object helpers.
- Do not route object emission through `parse_rv64_asm_line` or
  `encode_rv64_asm_line`; object emission must keep structured fragments.
- Do not move symbol kind mapping, relocation type mapping, local label
  binding, object module assembly, ELF writing, prepared data-object emission,
  inline-asm substitution/classification, or public object-route entrypoints.

## Suggested Next

Execute Step 2: expose only the safe helper set from the low-level RV64
encoding owner and update the duplicated object-route helpers to delegate
through compatibility wrappers. Keep all `RiscvEncodedFragment`, label, fixup,
prepared, module, parser, and ELF behavior in the current owners.

## Watchouts

- Keep this behavior-preserving: no object byte, relocation, label, fixup,
  unsupported-contract, expectation, or RV64 capability changes.
- Do not move symbol kind mapping, relocation mapping, label binding, object
  module assembly, ELF writing, or prepared data-object emission.
- Do not route object emission through text parsing.
- The `rv64_line_assembler.hpp` public surface currently includes parser
  variants. If Step 2 cannot expose the pure helpers there without making the
  API look parser-owned, stop and ask the supervisor for a narrower shared
  helper file before creating one.
- The object-route `encode_u_type` currently accepts `std::uint32_t imm20`,
  while the line assembler copy accepts `std::int32_t imm20`; preserve the
  exact masking behavior when unifying names/signatures.
- Because `encode_b_type`, `encode_j_type`, AUIPC/JALR call words, and LO12
  helpers consume this shared surface, proof should cover object bytes and at
  least one object runtime path if the implementation touches branch or call
  word emission beyond wrapper delegation.

## Proof

Mapping-only packet; no build or tests run and no `test_after.log` produced.

Exact validation command for the first implementation packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|rv64_roundtrip_contract|cli_riscv64_return_zero_writes_elf_obj|cli_riscv64_vrm_insn_d_source_obj)' > test_after.log 2>&1
```
