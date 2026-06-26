# RV64I plus c4c EV64 Roundtrip Contract

Supported extension boundary: RV64I plus c4c EV64 `.insn.d`.

Out of scope for this runbook: M, A, F, D, C, standard vector, CSR, system, and
compressed instructions. Unsupported extensions and directives must fail
closed.

The committed corpus must cover these RV64I families:

- upper-immediate: `lui`
- pc-relative: `auipc`
- jumps: `jal`, `jalr`
- branches: `beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu`
- loads: `lb`, `lh`, `lw`, `ld`, `lbu`, `lhu`, `lwu`
- stores: `sb`, `sh`, `sw`, `sd`
- register-immediate: `addi`, `slti`, `sltiu`, `xori`, `ori`, `andi`
- shift-immediate: `slli`, `srli`, `srai`, `slliw`, `srliw`, `sraiw`
- word-immediate: `addiw`
- register-register: `add`, `sub`, `sll`, `slt`, `sltu`, `xor`, `srl`,
  `sra`, `or`, `and`
- word-register: `addw`, `subw`, `sllw`, `srlw`, `sraw`
- operands: ABI register aliases, `x0`/`zero`, signed immediate edges,
  unsigned-looking immediates, labels, and local labels
- c4c EV64: `.insn.d`

EV64 `.insn.d` binary layout:

- bits `[6:0]`: fixed RISC-V 64-bit instruction length prefix `0x3f`
- bits `[11:7]`: destination vector register
- bits `[14:12]`: reserved subformat, encoded as zero
- bits `[19:15]`: lhs vector register
- bits `[24:20]`: rhs vector register
- bits `[31:25]`: `.insn.d` major, 7-bit
- bits `[39:32]`: operation, 8-bit
- bits `[44:40]`: accumulator/fourth vector operand (`rs4`/mask)
- bits `[47:45]`: reserved, encoded as zero and decoded fail-closed
- bits `[63:48]`: dtype immediate, 16-bit

Canonical roundtrip route:

```text
input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o
```

Acceptance contract:

- `pass1.s == pass2.s`
- `pass2.o == pass3.o`
- no external assembler or objdump is the source of truth
- unsupported bytes, directives, encodings, and extensions fail closed

Current unsupported gaps for Step 1:

- c4c-as only accepts the existing small canonical subset, not all listed RV64I
  mnemonics.
- c4c-as does not yet parse the full label/fixup surface required by branches,
  jumps, and PC-relative forms in this corpus.
- c4c-objdump only decodes the existing small canonical subset, not full RV64I.
- full object and text stability for this corpus is scaffolding-only until
  later implementation steps expand assembler, relocation, and objdump support.
