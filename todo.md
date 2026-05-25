Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The Printing And Effect-Publication Boundary

# Current Packet

## Just Finished

Lifecycle review rejected source-idea closure after the completed
call-argument preservation lookup checkpoint. The source idea remains open, and
the active runbook is reset to the next specific checkpoint:
`calls_printing.cpp` call printing and effect publication.

## Suggested Next

Execute Step 1 by classifying the exported `calls_printing.cpp` helpers as
emission-node construction, pure machine-printer work, effect publication, or
duplicate prepared-call/effect decision logic.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into whole dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific register, memory, frame-slot, immediate, and assembly
  spelling in AArch64 code; the target of this checkpoint is ownership of
  printer and effect-publication responsibilities.
- Keep source-idea progress tied to deleted duplication, moved ownership, or a
  sharper emission-only boundary.

## Proof

No new code proof for this lifecycle-only reset. The just-completed broader
backend checkpoint passed after roll-forward in canonical `test_before.log`.
