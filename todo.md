Status: Active
Source Idea Path: ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Inline Asm Memory Effect Consumers

# Current Packet

## Just Finished

Activation created the active runbook and executor scratchpad for Step 1.

## Suggested Next

Execute `plan.md` Step 1: inventory inline-asm memory/address effect consumers
in `src/backend/prealloc/stack_layout/inline_asm.cpp` and related stack-layout
hints. Record structured metadata consumers, raw summary paths, placement
consequences, and proof gaps here while keeping implementation files unchanged
for the inventory packet.

## Watchouts

- Do not leave raw instruction-shape scans as undocumented semantic authority.
- Keep stack placement, register homes, and operand storage decisions in
  prealloc.
- Treat retained unstructured summaries as conservative placement policy, not
  BIR memory provenance authority.

## Proof

Activation is lifecycle-only. Run `git diff --check -- plan.md todo.md` for the
activation proof.
