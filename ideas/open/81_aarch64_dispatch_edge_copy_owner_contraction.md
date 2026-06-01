# AArch64 Dispatch Edge-Copy Owner Contraction

## Goal

Contract the remaining AArch64 edge-copy owner after the prepared edge-source
and typed stack-source authority routes, deciding which helpers belong in
`dispatch.cpp` and which belong in narrower consumer owners.

## Why This Exists

`dispatch_edge_copies.cpp` remains large even after ideas 47, 62-64, and 78
moved or consumed the important prepared edge-publication, join-copy, and typed
stack-source facts. The remaining code should now be mostly target-local
edge-copy emission, producer-context lookup, stack-source materialization, and
parallel-copy routing.

The cleanup question is owner layout, not another immediate BIR migration.

## Owned Files

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Narrow destination owners only if a helper clearly emits their records:
  `memory.cpp`, `comparison.cpp`, `alu.cpp`, or publication-related owners.

## In Scope

- Inventory edge-copy helpers by responsibility:
  producer-context lookup, source fact validation, load-local emission,
  typed-stack-source emission, block-entry move filtering, and select parallel
  copy lowering.
- Fold orchestration-only helpers into `dispatch.cpp` if they are not useful as
  an independent owner.
- Move target-local emission helpers only when the destination owner naturally
  owns the emitted record family.
- Preserve the prepared source facts from shared BIR/prealloc authority.

## Out Of Scope

- Replacing prepared edge-copy facts with local re-derivation.
- Adding x86 or RISC-V consumers.
- Combining this with `dispatch_publication.cpp` relocation.
- Changing edge-copy ordering behavior.

## Proof Expectations

- Focused backend tests for AArch64 edge copies, join parallel copies, typed
  stack-source edge publication, select parallel-copy sources, and predecessor
  publication.
- Before/after regression guard when the file split changes externally visible
  lowering behavior.

## Reviewer Reject Signals

- The route moves code only to reduce file count while making ownership harder
  to explain.
- Edge-copy source selection starts bypassing prepared source facts.
- The route weakens edge-copy or block-entry publication expectations.
- New testcase-shaped shortcuts are added for one edge-copy form.

