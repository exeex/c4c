Status: Active
Source Idea Path: ideas/open/215_aarch64_first_machine_node_selection_slice.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Existing Machine Node Selection Inputs

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized this execution
state from `ideas/open/215_aarch64_first_machine_node_selection_slice.md`.

## Suggested Next

Start Step 1 by inspecting accepted AArch64 target-MIR records, the
machine-node contract, and focused backend tests. Record the first safe
selection subset here before implementation edits.

## Watchouts

- Do not emit or parse assembly text under this plan.
- Select only from accepted target-MIR records; do not bypass through rendered
  BIR, old examples, or fixture names.
- Keep the first subset small and fail closed for unsupported records.
- Consume allocation, scratch, spill slot, memory operand, and provenance facts
  from accepted records instead of recomputing them locally.

## Proof

Activation-only lifecycle work. No build required.
