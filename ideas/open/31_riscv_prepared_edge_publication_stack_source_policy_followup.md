# RISC-V Prepared Edge Publication Stack Source Policy Follow-Up

## Goal

Broaden RISC-V prepared edge-publication support for stack-source
`StackSlot -> Register` moves beyond the concrete-offset 4-byte `lw` and
8-byte `ld` forms closed by ideas 25 and 28.

## Why This Exists

Idea 28 intentionally accepted one safe additional stack-source form through
shared `edge_publications`: concrete-offset, 8-byte stack sources loaded into
register destinations with `ld` when the offset fits a signed 12-bit RISC-V
load immediate. That leaves real policy questions for sub-word extension,
unsigned or floating 32-bit values, dynamic stack addressing, large offsets,
and aggregate-width sources. Those should remain durable follow-up work rather
than being silently folded into the closed 8-byte slice.

## In Scope

- Decide which remaining stack-source forms are safe to support next, such as
  sub-word integer loads, unsigned or floating 32-bit loads, large-offset
  stack loads, dynamic-address stack sources, or aggregate-width sources.
- Define target-local RISC-V load, extension, scratch-register, and address
  materialization policy before accepting any new form.
- Preserve shared prepared `edge_publications` as the only semantic authority
  for edge moves.
- Keep RISC-V stack-source load and address policy inside the RISC-V backend.
- Add focused positive and negative coverage for each newly supported
  stack-source policy.

## Out Of Scope

- `PointerBasePlusOffset -> Register` policy broadening; that belongs to
  `ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md`.
- Source-to-`StackSlot` destination policy broadening; that belongs to
  `ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md`.
- RISC-V-local rediscovery of edge-copy facts from predecessor or successor
  block shape.
- Moving RISC-V load, extension, scratch, or address materialization policy
  into shared prepare, BIR, or target-neutral helpers.
- Broad frame-layout or memory-layout rewrites unrelated to prepared
  edge-publication stack-source loads.

## Acceptance Criteria

- At least one remaining stack-source form is implemented through shared
  `edge_publications` lookup authority, or the route records a concrete policy
  reason the candidate form must remain fail-closed.
- Existing concrete-offset 4-byte `lw` and 8-byte `ld` stack-source behavior
  remains supported.
- Tests fail if shared publication facts are missing or ignored.
- Unsupported stack-source forms remain explicit and fail closed.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch matches fixture labels, value ids, stack slot ids, offsets, or
  test names instead of implementing a semantic stack-source policy.
- The patch scans predecessor or successor blocks or creates a RISC-V-local
  edge-copy fact table instead of consuming shared `edge_publications`.
- The patch weakens or removes the already-supported 4-byte `lw` or 8-byte
  `ld` stack-source paths to claim progress on another form.
- The patch claims sub-word, unsigned, floating, dynamic-address, large-offset,
  or aggregate stack-source support without explicit target-local load,
  extension, scratch, and address policy.
- The patch moves RISC-V stack-source policy into shared prepare, BIR, or
  target-neutral helpers.
- The patch is only a helper rename, expectation rewrite, or classification
  change while retaining the same fail-closed behavior for the selected form.
