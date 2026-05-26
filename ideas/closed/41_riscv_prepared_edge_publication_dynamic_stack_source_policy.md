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

## Closure Evidence

Closed as a documented fail-closed prepared-authority blocker.

Inventory found that RISC-V prepared edge publication has authority for
existing concrete `StackSlot -> Register` stack-source loads with 4-byte and
8-byte widths, including large concrete offsets through target-local `t6`
address materialization. It did not find shared prepared authority for a
dynamic stack-source base anchor, offset expression or source memory access,
load width tied to that dynamic address, or scratch-register contract.

The route preserved existing concrete stack-source behavior and made the
unsupported dynamic-address policy explicit in focused tests. Dynamic-shaped
`StackSlot` homes without concrete `offset_bytes` remain
`UnsupportedSourceHome`; adding pointer-base fields to such a `StackSlot` does
not reclassify it as `PointerBasePlusOffset`; and `PointerBasePlusOffset`
remains pointer/address-value materialization rather than a dynamic
stack-source load.

Validation evidence:
- Focused RISC-V prepared edge-publication and shared prepared lookup proof
  passed 2/2 after the fail-closed policy packet.
- Backend bucket proof passed 163/163 during Step 4 validation.
- Focused searches of `src/backend/mir/riscv/codegen/emit.cpp` found no
  dynamic stack-source load implementation and no local predecessor/successor
  edge-copy rediscovery in RISC-V edge-publication lowering.
- Regression guard read-only close check passed against the accepted backend
  log with 163 passed, 0 failed, and no new failing tests.
