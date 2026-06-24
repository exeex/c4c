Status: Active
Source Idea Path: ideas/open/340_rv64_standard_insn_inline_asm_stage1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Inline-Asm And RV64 Object Route

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad. No executor packet has
completed yet.

## Suggested Next

Delegate Step 1 to an executor to map the existing inline-asm carrier, RV64
backend/object route, and the narrowest byte-level proof harness.

## Watchouts

- Keep the active route limited to standard RV64 `.insn` scalar constraints.
- Do not absorb vector constraints, EV `.insn.d`, named operands, or consteval
  asm string work into this child.
- Treat external assembler proof, testcase-shaped string matching, or
  expectation downgrades as route drift.

## Proof

Lifecycle-only activation; no build or test proof was run.
