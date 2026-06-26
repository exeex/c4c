Status: Active
Source Idea Path: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove LLVM And c4c Roundtrip Compatibility

# Current Packet

## Just Finished

Step 4 proved LLVM and c4c roundtrip compatibility for the committed EV64
`.insn.d` prefixed layout.

Artifacts:

- Proof log: `build/agent_state/385_step4/proof.log`.
- Assertion log: `build/agent_state/385_step4/assertions.log`.
- C-source object:
  `build/agent_state/385_step4/riscv64_vrm_insn_d_source.c4c.o`.
- c4c-as canonical fixture:
  `build/agent_state/385_step4/canonical_insn_d.s`.
- c4c-as object:
  `build/agent_state/385_step4/canonical_insn_d.c4c-as.o`.
- c4c objdump output assembly:
  `build/agent_state/385_step4/canonical_insn_d.objdump.s`.
- c4c roundtrip object:
  `build/agent_state/385_step4/canonical_insn_d.roundtrip.o`.
- LLVM disassembly:
  `build/agent_state/385_step4/canonical_insn_d.llvm-objdump.log`.

Byte assertions:

- C-source object `.text`:
  `3f0320140b0403001305000067800000`.
- c4c-as canonical object `.text`:
  `3f0320140b0403001305000067800000`.
- c4c objdump -> c4c-as roundtrip `.text`:
  `3f0320140b0403001305000067800000`.
- Canonical EV64 word for
  `.insn.d 10, 11, v6, v0, v2, v4, 3`:
  `3f0320140b040300`.
- First byte is `0x3f`; low seven bits `[6:0] = 0x3f`.

c4c roundtrip result:

- `build/c4c-objdump` extracted the canonical object and printed:
  `.insn.d 10, 11, v6, v0, v2, v4, 3`.
- Reassembling the extracted assembly with `build/c4c-as` reproduced the same
  `.text` bytes exactly.

LLVM result:

- Command:
  `llvm-objdump -d --triple=riscv64 build/agent_state/385_step4/canonical_insn_d.c4c-as.o`.
- LLVM disassembly shows one EV64 row:
  `0: 1420033f 0003040b <unknown>`.
- The next decoded row is address `8` (`li a0, 0x0`); there is no address `4`
  split row.
- Conclusion: LLVM consumes the c4c-emitted EV64 bytes as one 8-byte unknown
  instruction, matching the Step 1 prefix contract.

## Suggested Next

Run the plan-owner close decision for idea 385. The executor runbook evidence is
exhausted: Step 1 proved the independent LLVM prefix behavior, Step 3
implemented the c4c layout, and Step 4 proved c4c plus LLVM compatibility on
fresh emitted objects.

## Watchouts

- Preserve the source idea's scope: EV64 `.insn.d` length-prefix layout only.
- Do not implement unrelated RV64 object-route lowering or upstream LLVM
  decoder-table support.
- Keep the existing seven-operand `.insn.d` surface stable; this plan moves
  fields in the binary encoding and does not add/remove operands.
- c4c-objdump intentionally does not decode the old `major`-in-low-bits layout.
- No root regression logs were overwritten in Step 4; evidence lives under
  `build/agent_state/385_step4/`.

## Proof

Commands:

```bash
cmake --build --preset default --target c4cll c4c-as c4c-objdump
build/c4cll --codegen obj --target riscv64-linux-gnu tests/backend/case/riscv64_vrm_insn_d_source.c -o build/agent_state/385_step4/riscv64_vrm_insn_d_source.c4c.o
build/c4c-as build/agent_state/385_step4/canonical_insn_d.s -o build/agent_state/385_step4/canonical_insn_d.c4c-as.o
build/c4c-objdump build/agent_state/385_step4/canonical_insn_d.c4c-as.o -o build/agent_state/385_step4/canonical_insn_d.objdump.s
build/c4c-as build/agent_state/385_step4/canonical_insn_d.objdump.s -o build/agent_state/385_step4/canonical_insn_d.roundtrip.o
llvm-objdump -d --triple=riscv64 build/agent_state/385_step4/canonical_insn_d.c4c-as.o
```

Result: passed. `build/agent_state/385_step4/assertions.log` records:
`assertion=PASS c4c emitted prefixed EV64 bytes, c4c roundtripped them, and
LLVM consumed the EV64 word as one 8-byte unknown instruction`.
