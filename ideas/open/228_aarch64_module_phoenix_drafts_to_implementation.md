# AArch64 Module Phoenix Stage 4 Drafts To Implementation

Status: Open
Created: 2026-05-14
Parent Context: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Requires: ideas/closed/227_aarch64_module_phoenix_replacement_drafts.md

## Intent

Convert the reviewed replacement drafts into real AArch64 module/MIR lowering
implementation, routing prepared BIR directly into MIR machine nodes through
the reviewed ownership seams.

Read this first before doing phoenix-stage work:
`.codex/skills/phoenix-rebuild/SKILL.md`.

## Stage In Sequence

Stage 4 of 4: staged implementation conversion, dispatcher rewiring, proof,
and legacy retirement.

## Produces

- Real `.cpp` / `.hpp` implementation matching the reviewed draft set.
- Dispatcher and ownership rewiring that routes behavior through the reviewed
  replacement seams.
- Explicit accounting of moved responsibilities and remaining legacy
  compatibility.
- Legacy deletion or retirement only after the new owner is live and proved.

## Does Not Yet Own

- Reopening the stage-2 layout without a lifecycle repair.
- Reinterpreting prepared authority semantics for frame, call, register
  allocation, spill/reload, storage plan, data, or control-flow facts.
- Broad instruction coverage unrelated to proving the migrated seam.
- Object encoding, ELF relocation emission, linker behavior, x86, or RISC-V.

## Unlocks

Closure review for the AArch64 module phoenix rebuild route and, if still
needed, a separate follow-up idea for any compatibility or target-printer work
left outside the rebuilt module/MIR lowering surface.

## Scope Notes

- Migrate in behavior-preserving slices rather than an all-at-once rewrite.
- Keep old live behavior available until the new dispatcher can choose the
  replacement path for the migrated seam.
- Typical migration order is shared helpers, value/home or operand resolution,
  one coherent lowering family, dispatch rewiring, then deletion of dead
  compatibility code.
- Every migration packet must state what responsibility moved, what still
  remains in legacy code, and what proof covers the moved seam.
- Proof must cover each migrated capability family before completion is
  claimed.

## Boundaries

- Do not weaken tests, mark supported paths unsupported, or accept
  expectation-only progress.
- Do not add testcase-shaped shortcuts or named-case-only lowering.
- Do not silently keep the exact old module-emitter failure mode behind a new
  abstraction name.
- Do not delete compatibility code until the replacement owner is live and
  proved.

## Completion Signal

This idea is complete only when the reviewed draft set has been converted into
real implementation, the new ownership seams are actually in use, remaining
legacy code is explicitly classified as compatibility or follow-up scope,
direct prepared-BIR-to-MIR machine-node lowering is the active route for the
migrated surface, and proof shows the migrated capability families still work.

## Reviewer Reject Signals

Reject the route if implementation diverges from reviewed drafts without a
lifecycle repair, skips staged proof, removes legacy code before the new owner
is live, weakens tests, adds named-case shortcuts, leaves direct lowering
unused behind dispatcher compatibility, or preserves the old module emitter
and record design as the real owner under replacement filenames.
