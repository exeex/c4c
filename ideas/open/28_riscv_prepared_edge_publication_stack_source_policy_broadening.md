# RISC-V Prepared Edge Publication Stack Source Policy Broadening

## Goal

Broaden RISC-V prepared edge-publication consumption for stack-slot sources
beyond the focused 4-byte concrete-offset `StackSlot -> Register` route.

## Why This Exists

Idea 25 intentionally closed with focused support for stack sources that have a
concrete offset and 4-byte size, emitted as `lw <dst>, <offset>(sp)`. The route
left stack sources without offsets, non-I32 widths, dynamic stack addressing,
and large-offset policy fail-closed because those surfaces require explicit
RISC-V load/addressing rules rather than being silently folded into the first
stack-source helper.

## In Scope

- Decide which additional stack-source forms are semantically safe to consume
  through shared prepared `edge_publications`.
- Define target-local RISC-V load, sign/zero extension, scratch, and address
  materialization policy for the selected stack-source forms.
- Preserve shared `edge_publications` as the only authority for edge moves.
- Add focused positive and negative coverage for the selected broadened stack
  source behavior.
- Keep unsupported stack-source forms explicit and fail closed.

## Out Of Scope

- `PointerBasePlusOffset -> Register` source homes; those belong to
  `ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md`.
- Source-to-`StackSlot` destination moves; those belong to
  `ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md`.
- RISC-V-local rediscovery of edge-copy facts from predecessor or successor
  block shape.
- Moving RISC-V stack-source addressing or load policy into shared prepare,
  BIR, or target-neutral helpers.
- Broad RISC-V codegen or frame-layout rewrites unrelated to prepared
  edge-publication stack-source loads.

## Acceptance Criteria

- At least one stack-source form beyond the idea 25 4-byte concrete-offset path
  is implemented through shared `edge_publications` lookup authority, or the
  route records a concrete policy reason it must remain fail-closed.
- Tests fail if shared publication facts are missing or ignored.
- Existing `Register -> Register`, `RematerializableImmediate -> Register`,
  and focused 4-byte `StackSlot -> Register` behavior remains supported.
- Unsupported stack-source forms remain explicit and fail closed.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch matches fixture labels, value ids, stack slot ids, offsets, or test
  names instead of implementing a semantic stack-source policy.
- The patch scans predecessor or successor blocks or creates a RISC-V-local
  edge-copy fact table instead of consuming shared `edge_publications`.
- The patch claims broad stack-source support while only preserving the exact
  idea 25 4-byte concrete-offset behavior under a new helper name.
- The patch weakens or marks existing register, immediate, or focused
  stack-source paths unsupported to claim progress.
- The patch moves RISC-V load/addressing or scratch policy into shared prepare,
  BIR, or target-neutral helpers.
- The patch claims pointer-base source or stack destination support without
  routing that separate surface through its own idea.
