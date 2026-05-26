# RISC-V Prepared Edge Publication Stack Destination Policy Broadening

## Goal

Broaden RISC-V prepared edge-publication support for source-to-`StackSlot`
destination moves beyond the focused `Register -> StackSlot` form implemented
by idea 27.

## Why This Exists

Idea 27 intentionally proved only the first stack-destination source family:
register sources stored into concrete 4-byte stack-slot destinations through
shared `edge_publications`. Immediate, stack-source, and pointer-base sources
need additional target-local materialization or store policy and should not be
silently claimed by the focused register-source slice.

## In Scope

- Decide which additional source-to-`StackSlot` homes are safe to support next,
  such as `RematerializableImmediate -> StackSlot`,
  `StackSlot -> StackSlot`, or `PointerBasePlusOffset -> StackSlot`.
- Keep shared prepared `edge_publications` as the only semantic authority for
  every supported stack-destination move.
- Add target-local RISC-V emission only after the source and destination facts
  are concrete enough to avoid local rediscovery.
- Preserve the existing `Register -> StackSlot` support from idea 27 and all
  existing register-destination consumers.
- Keep malformed stack destinations, non-move publications, missing authority,
  and unsupported homes explicit and fail closed.

## Out Of Scope

- RISC-V-local edge fact discovery from predecessor or successor block shape.
- Moving RISC-V store/addressing policy into shared prepare, BIR, or
  target-neutral helpers.
- Broad RISC-V frame-layout, dynamic stack, or memory-layout rewrites unrelated
  to prepared edge-publication destination stores.
- Pointer-base register-destination policy broadening; that belongs to
  `ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md`.
- Stack-source register-destination policy broadening; that belongs to
  `ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md`.

## Acceptance Criteria

- At least one additional source-to-`StackSlot` RISC-V prepared
  edge-publication form is implemented through shared lookup authority, or the
  route records the concrete RISC-V policy reason candidate forms must remain
  fail-closed and moves the required policy work into durable follow-up scope.
- Tests fail if shared publication facts are absent or ignored.
- Existing `Register -> StackSlot` behavior remains supported.
- Unsupported source or destination homes remain explicit and fail closed.
- Target-specific stack destination emission remains inside the RISC-V backend.
- Validation covers focused RISC-V stack-destination publication tests,
  relevant shared prepared lookup tests, and an appropriate backend bucket.

## Discovered Policy Blocker

Step 1/2 execution found that `RematerializableImmediate -> StackSlot`,
`StackSlot -> StackSlot`, and `PointerBasePlusOffset -> StackSlot` all require
a RISC-V scratch-register or scratch-reservation contract before the prepared
edge-publication consumer can emit them without guessing. Idea 30 may close as
a fail-closed policy slice after validation if that blocker remains explicit
and the scratch-register policy is tracked separately.

## Reviewer Reject Signals

- The patch matches testcase names, fixture labels, edge names, or prepared
  value ids instead of implementing a semantic source-to-stack rule.
- The patch scans predecessor or successor blocks or creates local edge-copy
  facts instead of consuming shared `edge_publications`.
- The patch weakens or removes existing `Register -> StackSlot` behavior to
  claim progress on another source home.
- The patch moves RISC-V store/addressing policy into shared prepare, BIR, or
  target-neutral helpers.
- The patch claims broad stack-destination support while only preserving the
  already-supported register-source path.
- The patch is only a helper rename, expectation rewrite, or classification
  change while the selected new source-to-stack form remains unsupported.
