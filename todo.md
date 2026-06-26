Status: Active
Source Idea Path: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Prove The 64-Bit Prefix Contract

# Current Packet

## Just Finished

Activated idea 385 after idea 354 closed and no active `plan.md`/`todo.md`
remained. `ideas/open/` contains only
`ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md`.

## Suggested Next

Execute Step 1: construct a minimal EV64 candidate 8-byte word with low 7 bits
`0x3f`, run it through LLVM RISC-V objdump, and record whether the disassembler
consumes it as one 8-byte unknown instruction.

## Watchouts

- Preserve the source idea's scope: EV64 `.insn.d` length-prefix layout only.
- Do not implement unrelated RV64 object-route lowering or upstream LLVM
  decoder-table support.
- Do not merely rewrite expected hex values; prove both c4c roundtrip and LLVM
  one-instruction consumption.
- Keep `.insn.d` parser validation and malformed operand rejection intact.

## Proof

Lifecycle activation checks performed:

- `git status --short` showed a clean worktree before activation.
- `plan.md` and `todo.md` were absent before activation.
- `ideas/open/` contained only
  `ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md`.
