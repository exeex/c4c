Status: Active
Source Idea Path: ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Stack Layout Reconstruction Routes

# Current Packet

## Just Finished

Activation created the active runbook and executor scratchpad for Step 1.

## Suggested Next

Execute `plan.md` Step 1: inventory stack-layout reconstruction routes in
`src/backend/prealloc/stack_layout/*.cpp`, including name-based family
inference, BIR instruction scans, address-publication hints, pointer roots, and
alloca/copy coalescing dependencies. Record findings here and keep
implementation files unchanged for the inventory packet.

## Watchouts

- Do not move frame offsets, stack object ordering, or home-slot placement into
  BIR.
- Do not treat every BIR scan as duplication without naming the reconstructed
  fact.
- Separate prealloc placement-only hints from target-neutral slice-family or
  publication facts.

## Proof

Activation is lifecycle-only. Run `git diff --check -- plan.md todo.md` for the
activation proof.
