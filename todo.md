Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define the structured LIR inline-asm value contract

# Current Packet

## Just Finished

- No implementation packet has completed under this runbook.

## Suggested Next

- Execute Plan Step 1 using the supervisor-selected focused LIR proof command.

## Watchouts

- Use ordinary LIR/BIR SSA values; do not create a separate
  `InlineAsmOperand` identity system.
- Preserve original semantic text separately from LLVM-compatible rendering.
- Do not absorb general opcode lowering, target constraint typing, regalloc,
  MIR, or the existing unrelated dirty README changes into Step 1.

## Proof

- Not run; lifecycle activation only.
