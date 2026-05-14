Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Review Stage 1 Evidence Shape

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and `todo.md` for Step 1 of the Stage 2
AArch64 module phoenix review/layout idea.

## Suggested Next

Delegate Step 1 to an executor to review the Stage 1 extraction evidence:
`src/backend/mir/aarch64/module/module.cpp.md`,
`src/backend/mir/aarch64/module/module.hpp.md`, and
`src/backend/mir/aarch64/module/module.md`.

## Watchouts

- Preserve the Stage 1 correction: `src/backend/mir/aarch64/module/module.cpp`
  remains compiled legacy evidence until replacement implementation plus build
  wiring exist.
- Stage 2 does not own implementation edits, build rewiring, test expectation
  changes, or replacement draft contents.
- Reject a route that rubber-stamps weak extraction, preserves the legacy module
  emitter under new names, or avoids direct prepared-BIR-to-MIR machine-node
  lowering.

## Proof

Lifecycle-only activation. No build or test proof required for this slice.
