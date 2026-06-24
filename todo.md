Status: Active
Source Idea Path: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Stage 1 Inline Asm Surfaces

# Current Packet

## Just Finished

Lifecycle activation created this Stage 2 runbook from the RV64 inline asm
custom vector encoding umbrella.

## Suggested Next

Delegate Step 1 to an executor: inspect the completed Stage 1 inline asm
implementation and identify the exact files/functions that own vector
constraint classification, allocation, substitution, and tests.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Keep Stage 2 focused on `VR`, `VRM2`, and `VRM4`; EV `.insn.d`,
  named-operands, masks, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, and raw-string constraint
  acceptance as route failures.

## Proof

Lifecycle-only activation. No build or test run was required.
