Status: Active
Source Idea Path: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map `.insn.d` Extension Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the Stage 3 runbook from the RV64 inline asm
custom vector encoding umbrella after Stage 2 was closed.

## Suggested Next

Delegate Step 1 to an executor: inspect the completed Stage 1 and Stage 2
inline asm routes and identify the exact files/functions that own `.insn.d`
classification, immediate extraction, register substitution, 64-bit object
emission, and focused proof tests.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Treat Stage 2 as completed foundation from
  `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`.
- Keep Stage 3 focused on positional `.insn.d`; named operands, masks, EV
  semantics, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, external-assembler proof, and
  raw-string-only acceptance as route failures.

## Proof

Lifecycle-only activation. No build or test run was required.
