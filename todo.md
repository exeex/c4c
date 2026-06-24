Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Type And Inline Asm Carrier Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the VRM carrier type regalloc-frontier runbook.

## Suggested Next

Delegate Step 1 to `c4c-executor`: map the existing builtin type, lowering,
and prepared inline asm carrier surfaces before choosing the implementation
representation.

## Watchouts

- Do not treat scalar values, `long`, or `i64` as vector registers.
- Do not reuse GCC vector metadata for VRM carriers.
- Do not drift into `.insn.d` encoding, EV instruction design, final register
  allocation, linker, relocation, or disassembler work.
- Keep scalar mismatch diagnostics active.

## Proof

Lifecycle-only activation. No code validation required.
