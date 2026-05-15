Status: Active
Source Idea Path: ideas/open/242_inline_asm_memory_address_constraints.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared-Home And Constraint Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/242_inline_asm_memory_address_constraints.md`.

## Suggested Next

Delegate Step 1 to an executor: inspect the existing inline-asm prepared-home,
BIR, AArch64 selection, printing, and test surfaces for memory/address
constraint authority.

## Watchouts

- Do not infer memory/address support from rendered assembly text.
- Do not add allocator, spill, scratch-register, clobber ingress, or tied-home
  coallocation policy in this route.
- If no supported representative is expressible with current prepared-home
  data, record that blocker precisely instead of weakening tests.

## Proof

Lifecycle-only activation; no code validation run.
