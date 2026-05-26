# RISC-V Prepared Edge Publication Aggregate Stack Source Policy

## Goal

Decide whether RISC-V prepared edge publications should support aggregate-width
`StackSlot -> Register` stack-source moves, and implement the first safe form
only with explicit copy-width, register, and scratch policy.

## Why This Exists

Idea 31 kept aggregate-width stack-source moves fail-closed while adding
large-offset scalar loads. Aggregates need different authority than scalar
loads: copy size, lane/register destination semantics, scratch use, partial
copy rules, and ABI/layout constraints must be explicit before a RISC-V backend
sequence can be accepted.

## In Scope

- Audit aggregate-width `StackSlot -> Register` prepared publication facts and
  destination expectations visible to RISC-V lowering.
- Decide whether there is a narrow aggregate form with enough authority to
  emit safely.
- Define target-local load/copy sequencing, scratch-register, alignment, and
  fail-closed policy for the selected form.
- Preserve shared prepared `edge_publications` lookup as the only semantic
  authority for edge moves.
- Add focused positive and negative tests for the selected aggregate form or
  for the chosen fail-closed policy.

## Out Of Scope

- Typed scalar stack-source load and extension policy.
- Dynamic-address stack-source support.
- Source-to-`StackSlot` destination broadening.
- `PointerBasePlusOffset -> Register` broadening.
- Broad ABI, frame-layout, or aggregate layout rewrites outside the selected
  prepared edge-publication source form.
- Recreating edge-copy facts by scanning predecessor or successor blocks.

## Acceptance Criteria

- One aggregate-width `StackSlot -> Register` source form is implemented
  through shared `edge_publications`, or the route records a concrete policy
  reason aggregate stack sources must remain fail-closed.
- Existing scalar 4-byte, 8-byte, and large-offset stack-source behavior
  remains supported.
- Unsupported aggregate widths, destination register expectations, partial
  copies, alignments, and scratch needs remain explicit and fail closed.
- Tests fail if shared publication facts are missing or ignored.
- Validation covers focused RISC-V prepared edge-publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch treats aggregate stack sources as scalar loads based only on byte
  size, fixture names, value ids, stack slot ids, offsets, or test names.
- The patch claims aggregate support without explicit copy-width, lane or
  destination-register, scratch-register, alignment, and partial-copy policy.
- The patch scans predecessor or successor blocks or creates a RISC-V-local
  edge-copy fact table instead of consuming shared `edge_publications`.
- The patch weakens existing scalar or large-offset stack-source behavior to
  claim aggregate progress.
- The patch hides the old aggregate fail-closed behavior behind helper renames,
  expectation rewrites, or classification-only changes.
- The patch broadens dynamic-address, typed-scalar, pointer-base, or
  source-to-stack behavior outside this idea.
