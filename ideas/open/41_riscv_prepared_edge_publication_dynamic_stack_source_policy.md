# RISC-V Prepared Edge Publication Dynamic Stack Source Policy

## Goal

Define whether and how RISC-V prepared edge publications can load
`StackSlot -> Register` sources whose effective address is dynamic or otherwise
not represented as a concrete stack offset.

## Why This Exists

Idea 31 intentionally supported only concrete stack offsets, including large
offsets through target-local address materialization. Dynamic stack sources
need a separate policy for address authority, frame or stack anchor selection,
scratch-register use, and fail-closed diagnostics before they can be emitted
without rediscovering edge-copy facts in RISC-V codegen.

## In Scope

- Inventory dynamic stack-source homes visible to RISC-V prepared edge
  publication lowering.
- Decide what prepared authority must identify the dynamic address, base
  anchor, offset expression, and load width.
- Define target-local RISC-V address materialization and scratch-register
  policy for one safe dynamic stack-source form, if one exists.
- Preserve shared prepared `edge_publications` lookup as the only semantic
  authority for edge moves.
- Add focused positive and negative tests for the selected dynamic form or for
  the chosen fail-closed policy.

## Out Of Scope

- Typed scalar extension policy for concrete stack offsets.
- Aggregate-width stack-source support.
- Source-to-`StackSlot` destination broadening.
- `PointerBasePlusOffset -> Register` broadening.
- Broad dynamic-stack frame-layout rewrites not required for edge-publication
  source addressing.
- Recreating edge-copy facts by scanning predecessor or successor blocks.

## Acceptance Criteria

- One dynamic-address `StackSlot -> Register` source form is implemented
  through shared `edge_publications`, or the route records a concrete prepared
  authority reason dynamic stack sources must remain fail-closed.
- Existing concrete 4-byte, 8-byte, and large-offset stack-source behavior
  remains supported.
- Unsupported dynamic bases, missing address facts, widths, and scratch needs
  remain explicit and fail closed.
- Tests fail if shared publication facts are missing or ignored.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch derives dynamic stack addresses from block shape, predecessor or
  successor scans, fixture labels, value ids, stack slot ids, or test names.
- The patch assumes a concrete `sp` offset when the prepared home does not
  provide one.
- The patch claims dynamic support without explicit base-anchor, address
  materialization, load-width, and scratch-register policy.
- The patch weakens existing concrete-offset or large-offset stack-source
  support to claim dynamic-address progress.
- The patch moves dynamic stack-source policy into unrelated shared prepare,
  BIR, frame-layout, or target-neutral code without a separate source idea.
- The patch is only an expectation rewrite, helper rename, or classification
  change while retaining the same fail-closed dynamic-address behavior.
