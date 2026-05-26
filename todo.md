# Current Packet

Status: Active
Source Idea Path: ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Current Consumer And Scratch Constraints

## Just Finished

Lifecycle activation created the active runbook from the source idea.

## Suggested Next

Delegate Step 1 to an executor: inspect the RISC-V prepared edge-publication
consumer, existing `Register -> StackSlot` behavior, nearby scratch-register
conventions, and current fail-closed points for non-register sources into
`StackSlot` destinations.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority
  for supported edge moves.
- Do not use a hard-coded scratch register without an explicit ownership,
  reservation, or clobber contract.
- Do not match fixture labels, value ids, stack slot ids, offsets, or test
  names.
- Existing `Register -> StackSlot` behavior must remain supported.
- Unsupported source or destination homes must remain explicit and fail closed.

## Proof

Lifecycle-only activation. No build or test proof required for this packet.
