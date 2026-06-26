Status: Active
Source Idea Path: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Prove The 64-Bit Prefix Contract

# Current Packet

## Just Finished

Step 1 proved the candidate RISC-V 64-bit length-prefix behavior independently
of the current c4c EV64 encoder.

Fixture:

- Assembly artifact:
  `build/agent_state/385_step1/ev64_prefix_probe.s`
- Object artifact:
  `build/agent_state/385_step1/ev64_prefix_probe.o`
- Candidate `.text` bytes:
  `3f 00 00 00 00 00 00 00`
- Prefix assertion:
  first byte `0x3f`; low seven bits `[6:0] = 0x3f`.

LLVM objdump result:

- Command:
  `llvm-objdump -d --triple=riscv64 build/agent_state/385_step1/ev64_prefix_probe.o`
- Output log:
  `build/agent_state/385_step1/ev64_prefix_probe.llvm-objdump.log`
- Relevant disassembly:
  `0: 0000003f 00000000     <unknown>`
- There is no disassembly row at address `4`, so LLVM consumed the two raw
  32-bit chunks as one 8-byte unknown instruction.

Additional artifacts:

- ELF/readelf log:
  `build/agent_state/385_step1/ev64_prefix_probe.readelf.log` confirms
  `elf64-littleriscv` / RISC-V and `.text` size `0x8`.
- Assertion log:
  `build/agent_state/385_step1/ev64_prefix_probe.assertion.log` records:
  `low_7_bits=0x3f`, `unknown_lines=1`,
  `has_address_4_line=false`, and
  `assertion=PASS llvm consumed the 8 bytes as one unknown instruction`.
- GNU objdump contrast:
  `build/agent_state/385_step1/ev64_prefix_probe.gnu-objdump.log` splits the
  bytes into two `.word` rows; the Step 1 compatibility proof is specifically
  for LLVM RISC-V objdump, matching the source idea's acceptance concern.

## Suggested Next

Execute Step 2: map the current c4c EV64 `.insn.d` bit layout against the
proved LLVM prefix contract and choose the exact field remapping that preserves
operands while forcing bits `[6:0] = 0x3f`.

## Watchouts

- Preserve the source idea's scope: EV64 `.insn.d` length-prefix layout only.
- Do not implement unrelated RV64 object-route lowering or upstream LLVM
  decoder-table support.
- This packet proves only the independent LLVM one-instruction consumption
  contract. The c4c encoder/decoder roundtrip remains Step 2+ implementation
  work.
- Keep `.insn.d` parser validation and malformed operand rejection intact.

## Proof

Commands run:

- `mkdir -p build/agent_state/385_step1`
- `printf ... > build/agent_state/385_step1/ev64_prefix_probe.s`
- `clang --target=riscv64-linux-gnu -c build/agent_state/385_step1/ev64_prefix_probe.s -o build/agent_state/385_step1/ev64_prefix_probe.o`
- `llvm-readelf -h -S build/agent_state/385_step1/ev64_prefix_probe.o > build/agent_state/385_step1/ev64_prefix_probe.readelf.log`
- `llvm-objdump -d --triple=riscv64 build/agent_state/385_step1/ev64_prefix_probe.o > build/agent_state/385_step1/ev64_prefix_probe.llvm-objdump.log`
- `riscv64-linux-gnu-objdump -d build/agent_state/385_step1/ev64_prefix_probe.o > build/agent_state/385_step1/ev64_prefix_probe.gnu-objdump.log`
- Python assertion script wrote
  `build/agent_state/385_step1/ev64_prefix_probe.assertion.log`.

Result: passed. No implementation files, `plan.md`, source idea files, or root
regression logs were edited.
