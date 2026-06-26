Status: Active
Source Idea Path: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Map The New EV64 Field Layout

# Current Packet

## Just Finished

Step 2 mapped the current c4c EV64 `.insn.d` field layout to the new
RISC-V 64-bit length-prefix-compatible layout.

Surfaces inspected:

- Encoder/classifier for prepared inline asm:
  `src/backend/mir/riscv/codegen/object_emission.cpp`
  (`classify_prepared_rv64_insn_d_inline_asm`,
  `encode_rv64_ev_insn_d_inline_asm`).
- Standalone assembler line parser/encoder:
  `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
  (`parse_insn_d`, `encode_insn_d_word`).
- c4c objdump extractor:
  `src/apps/c4c-objdump.cpp` (`decode_insn_d`,
  `instruction_assembly`).
- Relevant tests/docs:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`,
  `tests/backend/CMakeLists.txt`,
  `tests/backend/cmake/run_c4c_as_parse_suite.cmake`,
  `tests/backend/cmake/run_c4c_objdump_extract_suite.cmake`,
  `tests/backend/rv64/rv64i_ev64_roundtrip_contract.md`,
  `tests/backend/rv64/rv64i_ev64_roundtrip_corpus.s`, and
  `tests/backend/case/riscv64_vrm_insn_d_source.c`.

Current layout:

| Bits | Current field |
| --- | --- |
| `[6:0]` | `.insn.d` `major` / namespace, currently `0x0a` in the canonical fixture. |
| `[11:7]` | destination vector register `rd`. |
| `[14:12]` | zero `funct3`. |
| `[19:15]` | lhs vector register `rs1`. |
| `[24:20]` | rhs vector register `rs2`. |
| `[29:25]` | accumulator/fourth vector register `rs3`. |
| `[31:30]` | zero `funct2`. |
| `[39:32]` | `.insn.d` `operation` / EV operation byte. |
| `[55:40]` | `.insn.d` `dtype` immediate. |
| `[63:56]` | zero. |

Required new layout:

| Bits | New field |
| --- | --- |
| `[6:0]` | fixed RISC-V 64-bit length prefix `0x3f`; not a user operand. |
| `[11:7]` | destination vector register `rd` from `.insn.d` destination operand. |
| `[14:12]` | zero reserved subformat/funct3 for the first EV64 format. |
| `[19:15]` | lhs vector register `rs1`. |
| `[24:20]` | rhs vector register `rs2`. |
| `[31:25]` | `.insn.d` `major` moved here as a 7-bit EV major/subformat field. |
| `[39:32]` | `.insn.d` `operation` remains the c4c/user opcode byte. |
| `[44:40]` | accumulator/fourth vector operand moves here as `rs4/msk`. |
| `[47:45]` | reserved, must encode as zero and decode fail-closed when nonzero. |
| `[63:48]` | `.insn.d` `dtype` immediate moves here as `dtype imm16`. |

Implementable masks/shifts:

- Fixed prefix: `(word & 0x7f) == 0x3f`.
- `destination = (word >> 7) & 0x1f`.
- Reserved subformat: `((word >> 12) & 0x7) == 0`.
- `lhs = (word >> 15) & 0x1f`.
- `rhs = (word >> 20) & 0x1f`.
- `major = (word >> 25) & 0x7f`.
- `operation = (word >> 32) & 0xff`.
- `accumulator = (word >> 40) & 0x1f`.
- Reserved high subformat: `((word >> 45) & 0x7) == 0`.
- `dtype = (word >> 48) & 0xffff`.

Canonical byte example:

- Old `.insn.d 10, 11, v6, v0, v2, v4, 3` bytes:
  `0a0320080b030000` (`0x0000030b0820030a`).
- New mapped bytes:
  `3f0320140b040300` (`0x0003040b1420033f`).
- The first byte becomes `0x3f`, satisfying the Step 1 LLVM length-prefix
  contract while preserving all seven public `.insn.d` operands.

Step 3 tests/docs to update:

- `backend_riscv_object_emission_test`: expected `read_u64` values for
  canonical and substituted `.insn.d` encodings, and any reject text that still
  says the first immediate is an opcode/namespace field.
- `backend_cli_riscv64_vrm_insn_d_source_obj` in
  `tests/backend/CMakeLists.txt`: `EXPECTED_HEX_CONTAINS`.
- `run_c4c_as_parse_suite.cmake`: canonical expected hex strings and mixed
  RV64I+EV64 expected hex.
- `run_c4c_objdump_extract_suite.cmake`: `expected_hex`, `unsupported_hex`,
  and extraction expectations.
- `tests/backend/rv64/rv64i_ev64_roundtrip_contract.md`: document the new
  field layout and fixed prefix dependency.
- Roundtrip corpus/source fixtures remain textually valid
  (`.insn.d 10, 11, v6, v0, v2, v4, 3`), but their expected object bytes must
  change.

## Suggested Next

Execute Step 3: update the EV64 `.insn.d` encoder/decoder/extractor and focused
tests/docs to the mapped layout, then prove c4c roundtrip plus LLVM
one-instruction consumption for the emitted canonical bytes.

## Watchouts

- Preserve the source idea's scope: EV64 `.insn.d` length-prefix layout only.
- Do not implement unrelated RV64 object-route lowering or upstream LLVM
  decoder-table support.
- Keep the existing seven-operand `.insn.d` surface stable; this plan moves
  fields in the binary encoding and does not add/remove operands.
- c4c-objdump should decode only the new `0x3f` prefixed layout after the
  migration unless Step 3 explicitly records a compatibility reason.
- Keep `.insn.d` parser validation and malformed operand rejection intact.

## Proof

Read-only mapping packet; no build or test command was run. No implementation
files, tests, docs, `plan.md`, source idea files, or root regression logs were
edited.
