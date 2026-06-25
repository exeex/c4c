Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Existing RV64 Inline Asm Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution scratchpad from
idea 349. No implementation packet has run yet.

## Suggested Next

Execute Step 1 by locating the current RV64 inline asm substitution, text
printing, object encoding, and narrow test surfaces.

## Watchouts

- Do not start ideas 350 or 351 before idea 349 provides the shared line core.
- Do not add new testcase-shaped `.insn.d` object special cases.
- Keep inline asm constraints and VRM allocation out of the line parser.

## Proof

Lifecycle-only activation. No build proof required.
