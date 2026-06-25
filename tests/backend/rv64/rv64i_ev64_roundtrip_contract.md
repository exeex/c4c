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
