# RV64 EV64 `.insn.d` RISC-V Length Prefix Alignment

Status: Closed
Type: Backend object emission design/repair idea
Closed By: EV64 `.insn.d` prefixed layout implementation and backend regression acceptance

## Context

c4c currently supports an EV64 `.insn.d` inline-asm/object-emission template for
64-bit instruction words. The current encoding places the c4c major opcode in
bits `[6:0]`, for example `.insn.d 10, 11, v6, v0, v2, v4, 3` emits a word whose
low byte is `0x0a`.

LLVM's RISC-V disassembler classifies instruction length from the low bits of
the first byte. A 64-bit RISC-V instruction-length slot is recognized when
bits `[6:0]` are `0b0111111` (`0x3f`). If c4c does not use that prefix,
`llvm-objdump` may split the EV64 word as smaller instructions instead of
consuming one 8-byte unknown instruction.

## Goal

Redesign the EV64 `.insn.d` binary layout so every emitted 64-bit instruction
word conforms to the RISC-V variable-length instruction prefix convention.

The primary compatibility contract is:

- EV64 `.insn.d` words must have bits `[6:0] = 0b0111111`.
- LLVM RISC-V disassembly of an EV64 word without c4c-specific decoder support
  should consume 8 bytes and print one unknown instruction, not two 32-bit
  instructions or a sequence of smaller fragments.

## Proposed Encoding Shape

Initial bit layout:

```text
63          48 47      45 44    40 39          32 31               7 6       0
+-------------+----------+--------+--------------+------------------+---------+
| dtype imm16 | reserved | rs4/msk | opcode8 user | rv32-like 25-bit | 0x3f    |
+-------------+----------+--------+--------------+------------------+---------+
```

Field intent:

- `opcode7 = 0b0111111` in bits `[6:0]` is fixed and exists only to declare a
  64-bit RISC-V instruction-length slot.
- bits `[31:7]` preserve a 25-bit payload shaped like the non-opcode portion of
  normal RISC-V 32-bit instructions, so existing register-position instincts
  remain usable where possible.
- `opcode8 user` in bits `[39:32]` is the c4c/user-defined EV operation space.
- `rs4/msk` in bits `[44:40]` carries the fourth register operand, intended for
  scalable mask/vector-mask style operands.
- bits `[47:45]` are reserved for future subformat or policy bits and must be
  emitted as zero until a follow-up idea claims them.
- `dtype imm16` in bits `[63:48]` carries the 16-bit dtype/immediate field.

The exact mapping inside the 25-bit payload should be documented in the
implementation plan before code changes. A likely starting point is the
standard R-type non-opcode positions: `rd`, `funct3/subformat`, `rs1`, `rs2`,
and remaining function bits.

## In Scope

- Update the c4c EV64 `.insn.d` encoder to set the RISC-V 64-bit length prefix.
- Update the c4c EV64 `.insn.d` decoder/objdump extractor to understand the new
  layout.
- Update tests that currently assert the old `0x...0a` low opcode layout.
- Add proof that LLVM RISC-V objdump treats emitted EV64 bytes as one 8-byte
  unknown instruction when no c4c-specific decoder is present.
- Document the new field layout near the encoder and in the RV64 roundtrip
  contract.

## Out Of Scope

- Adding upstream LLVM decoder-table support for c4c EV64 instructions.
- General support for arbitrary RISC-V long-instruction encodings beyond this
  EV64 `.insn.d` template.
- Reinterpreting existing RV64I 32-bit instruction encodings.
- Adding semantic lowering for new EV operations beyond preserving the existing
  `.insn.d` inline-asm/object-emission behavior.

## Acceptance

- The canonical `.insn.d` fixture emits an 8-byte word whose low 7 bits are
  `0x3f`.
- c4c's own objdump/extractor roundtrips the canonical `.insn.d` spelling using
  the new layout.
- Existing prepared inline-asm `.insn.d` operands still allocate and substitute
  vector register operands correctly.
- A narrow LLVM objdump proof shows the EV64 bytes are consumed as one unknown
  8-byte RISC-V instruction, not split into smaller instructions.
- The RV64 roundtrip contract documents the new EV64 layout and explicitly
  calls out the RISC-V length-prefix dependency.

## Closure Notes

Idea 385's acceptance criteria are satisfied.

Step 1 proved the independent LLVM RISC-V objdump contract: a candidate
8-byte word with low seven bits `0x3f` is consumed as one unknown instruction
rather than split into smaller fragments.

Step 2 mapped the implemented EV64 `.insn.d` layout:

- prefix `[6:0] = 0x3f`
- destination `[11:7]`
- reserved `[14:12] = 0`
- lhs `[19:15]`
- rhs `[24:20]`
- major `[31:25]`
- operation `[39:32]`
- accumulator/rs4 `[44:40]`
- reserved `[47:45] = 0`
- dtype `[63:48]`

Implementation commit `11b36c5f` updated c4c EV64 encoders, the c4c-as line
encoder, c4c-objdump extractor, focused tests, and roundtrip contract
documentation. c4c-objdump intentionally decodes only the new prefixed layout
and rejects nonzero reserved `[14:12]` or `[47:45]` bits.

Step 4 artifacts under `build/agent_state/385_step4/` prove the canonical
fixture `.insn.d 10, 11, v6, v0, v2, v4, 3` now emits bytes
`3f0320140b040300`; c4c-objdump prints the canonical `.insn.d` spelling;
reassembly reproduces identical bytes; and LLVM objdump shows one unknown row
at address `0` with the next row at `8` and no address `4` split.

Step 5 backend proof ran:

`ctest --test-dir build -L backend -j --output-on-failure > test_after.log 2>&1`

Result: 326/326 backend tests passed. The supervisor regression guard against
`test_before.log` passed 326/326 before and after, and the accepted
`test_after.log` was rolled into `test_before.log`.

The plan-owner close gate was rerun as lifecycle-only validation against the
accepted current backend baseline on both sides:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: PASS, 326/326 before and 326/326 after, no new failures.

## Reviewer Reject Signals

Reject the route if any of these happen:

- The new encoder still permits EV64 words whose bits `[6:0]` are not `0x3f`.
- Tests only rewrite expected hex values without proving LLVM objdump consumes
  the word as one 8-byte unknown instruction.
- The implementation adds named-case handling for only the current fixture
  instead of changing the general `.insn.d` field layout.
- The old low-7-bit major-opcode layout remains reachable behind a renamed
  helper or compatibility path without an explicit migration reason.
- The route weakens `.insn.d` parser validation or accepts malformed register
  banks/immediates to make existing tests pass.
- The work expands into unrelated RV64 object-route lowering or LLVM upstream
  decoder implementation before this encoding contract is proven.
