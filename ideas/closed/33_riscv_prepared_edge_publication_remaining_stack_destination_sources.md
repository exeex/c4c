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

## Closure Note

Closed after accepting the narrow RISC-V I32 `StackSlot -> StackSlot` stack
destination policy. The implemented path consumes shared prepared
`edge_publications` as authority, emits the target-local `lw t0` plus `sw t0`
sequence for non-aliasing signed-12-bit stack offsets, preserves existing
`Register -> StackSlot` and `RematerializableImmediate -> StackSlot` I32
support, and keeps neighboring unsupported widths, offsets, source homes,
pointer-base sources, malformed homes, overlapping slots, missing publications,
and non-move publications fail-closed.

The source acceptance criterion required at least one remaining
source-to-`StackSlot` form to be implemented or a concrete fail-closed policy
reason to be recorded. The accepted `StackSlot -> StackSlot` policy satisfies
that bar; `PointerBasePlusOffset -> StackSlot` remains deliberately outside
this closed slice rather than silently broadening the idea.

Close validation used the accepted backend guard and full-suite baseline at
commit `cab18077d`: focused and backend proof passed, `test_after.log` records
163/163 backend tests passed, and `test_baseline.log` records 3411/3411 full
suite tests passed. The close-time regression guard comparison of
`test_before.log` to `test_after.log` passed with no new failures under the
non-decreasing pass-count policy.
