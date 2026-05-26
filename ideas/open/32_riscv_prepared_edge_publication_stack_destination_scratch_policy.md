# RISC-V Prepared Edge Publication Stack Destination Scratch Policy

## Goal

Define and implement the RISC-V scratch-register or scratch-reservation policy
needed before prepared edge-publication moves from non-register sources into
`StackSlot` destinations can be supported.

## Why This Exists

Idea 30 found that `RematerializableImmediate -> StackSlot`,
`StackSlot -> StackSlot`, and `PointerBasePlusOffset -> StackSlot` cannot be
emitted safely by the current prepared edge-publication consumer. Each form
needs a temporary register for immediate materialization, stack loads before
stores, pointer address materialization, or large-offset addressing. The
current helper has no owned scratch-register contract for edge moves, and a
hard-coded register would be an unreviewed target-local policy decision.

## In Scope

- Choose a target-local RISC-V scratch-register or scratch-reservation contract
  for prepared edge-publication edge moves.
- Decide which first source-to-`StackSlot` form can use that contract safely,
  such as `RematerializableImmediate -> StackSlot`,
  `StackSlot -> StackSlot`, or `PointerBasePlusOffset -> StackSlot`.
- Keep shared prepared `edge_publications` as the only semantic authority for
  each supported move.
- Keep stack-destination store, materialization, address, width, offset-range,
  and scratch policy inside the RISC-V backend.
- Add focused positive and negative coverage for any newly supported
  source-to-stack form.

## Out Of Scope

- RISC-V-local rediscovery of edge-copy facts from predecessor or successor
  block shape.
- Moving RISC-V scratch, load, store, address, or materialization policy into
  shared prepare, BIR, or target-neutral helpers.
- Stack-source register-destination policy broadening.
- Pointer-base register-destination policy broadening.
- Broad frame-layout, allocator, dynamic-stack, or memory-layout rewrites not
  required to define the prepared edge-publication scratch contract.

## Acceptance Criteria

- A concrete scratch-register or scratch-reservation policy is documented in
  the RISC-V prepared edge-publication consumer path.
- At least one non-register source-to-`StackSlot` form is implemented through
  shared `edge_publications` lookup authority using that scratch policy, or the
  route records why every candidate still must remain fail-closed.
- Existing `Register -> StackSlot` behavior remains supported.
- Unsupported source or destination homes remain explicit and fail closed.
- Tests fail if shared publication facts are absent or ignored.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch uses a hard-coded scratch register without an explicit RISC-V
  ownership, reservation, or clobber contract.
- The patch matches fixture labels, value ids, stack slot ids, offsets, or test
  names instead of implementing semantic source-to-stack lowering.
- The patch scans predecessor or successor blocks or creates local edge-copy
  facts instead of consuming shared `edge_publications`.
- The patch weakens or removes existing `Register -> StackSlot` support to
  claim progress on another source home.
- The patch claims immediate, stack-source, pointer-base, large-offset, or
  non-4-byte stack-destination support without explicit target-local scratch,
  materialization, store-width, and address policy.
- The patch moves RISC-V scratch, store, or address policy into shared prepare,
  BIR, or target-neutral helpers.
- The patch is only a helper rename, expectation rewrite, or classification
  change while retaining the same fail-closed behavior for the selected form.
