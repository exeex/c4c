# RISC-V Prepared Edge Publication Remaining Stack Destination Sources

## Goal

Broaden RISC-V prepared edge-publication support for the remaining
source-to-`StackSlot` destination forms left fail-closed by idea 32:
`StackSlot -> StackSlot` and `PointerBasePlusOffset -> StackSlot`.

## Why This Exists

Idea 32 defined the first stack-destination scratch contract and used it to
support `RematerializableImmediate -> StackSlot` I32 through shared
`edge_publications` authority. The two neighboring forms still need additional
target-local policy before they can be emitted safely: stack-source moves need
load plus store sequencing and scratch/address rules, while pointer-base
sources need pointer materialization, aliasing, and large-offset policy.

## In Scope

- Decide which remaining source-to-`StackSlot` form is safe to support next:
  `StackSlot -> StackSlot` or `PointerBasePlusOffset -> StackSlot`.
- Define target-local RISC-V load, store, address materialization, scratch, and
  large-offset policy before accepting any new form.
- Preserve shared prepared `edge_publications` as the only semantic authority
  for edge moves.
- Keep RISC-V source load, pointer materialization, stack-destination store,
  and address policy inside the RISC-V backend.
- Add focused positive and negative coverage for each newly supported form.

## Out Of Scope

- Reworking the immediate-to-stack support closed by idea 32 except to preserve
  it.
- RISC-V-local rediscovery of edge-copy facts from predecessor or successor
  block shape.
- Moving RISC-V load, store, scratch, address, or pointer materialization
  policy into shared prepare, BIR, or target-neutral helpers.
- Broad frame-layout, allocator, dynamic-stack, or memory-layout rewrites not
  required for the selected remaining source-to-stack form.
- Stack-source or pointer-base register-destination broadening outside the
  existing dedicated follow-up ideas.

## Acceptance Criteria

- At least one remaining source-to-`StackSlot` form is implemented through
  shared `edge_publications` lookup authority, or the route records a concrete
  policy reason the candidate must remain fail-closed.
- Existing `Register -> StackSlot` and `RematerializableImmediate -> StackSlot`
  I32 behavior remains supported.
- Unsupported source or destination homes, widths, offsets, and address forms
  remain explicit and fail closed.
- Tests fail if shared publication facts are missing or ignored.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch matches fixture labels, value ids, stack slot ids, offsets, or
  test names instead of implementing semantic source-to-stack lowering.
- The patch scans predecessor or successor blocks or creates a RISC-V-local
  edge-copy fact table instead of consuming shared `edge_publications`.
- The patch weakens or removes existing `Register -> StackSlot` or
  `RematerializableImmediate -> StackSlot` behavior to claim progress on a
  remaining form.
- The patch claims `StackSlot -> StackSlot`, `PointerBasePlusOffset ->
  StackSlot`, large-offset, non-4-byte, aggregate, or dynamic-address support
  without explicit target-local load, pointer materialization, scratch, store,
  aliasing, and address policy as applicable.
- The patch moves RISC-V stack-destination policy into shared prepare, BIR, or
  target-neutral helpers.
- The patch is only a helper rename, expectation rewrite, or classification
  change while retaining the same fail-closed behavior for the selected form.
