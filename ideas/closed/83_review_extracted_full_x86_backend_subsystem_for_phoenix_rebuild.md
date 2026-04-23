# Review Extracted Full X86 Backend Subsystem For Phoenix Rebuild

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Parent Idea: [82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md)

## Intent

Review the full `src/backend/mir/x86/` extraction package and define one
replacement layout that covers the root dispatcher, assembler, linker, and
already-extracted codegen tree as one subsystem.

## Stage In Sequence

Stage 2 of 4: extraction review and replacement layout.

## Produces

- `docs/backend/x86_subsystem_rebuild_plan.md`
- `docs/backend/x86_subsystem_rebuild_handoff.md`

## Does Not Yet Own

This stage does not own:

- drafting replacement file contents
- converting drafts into real implementation
- deleting legacy live code before replacement owners exist

## Unlocks

This stage unlocks the exact replacement file map and handoff contract that
stage 3 must draft for the whole `x86` subsystem.

## Scope Notes

- Read `.codex/skills/phoenix-rebuild/SKILL.md` first before doing stage-2
  Phoenix work on this idea.
- Review the full stage-1 package under `docs/backend/x86_subsystem_legacy/`
  together with the accepted `docs/backend/x86_codegen_legacy/` evidence.
- Judge whether the extraction set itself needs correction, expansion,
  compression, splitting, merging, or reorganization before stage 3 should
  trust it.
- Define one concrete replacement layout that addresses the historical
  backend-capability pressure that originally pushed the codegen-only Phoenix
  rebuild.

## Boundaries

Do not draft replacement implementation content in this stage. Keep it to
review, extraction correction judgment, layout design, and stage-3 handoff.

## Completion Signal

This idea is complete when `docs/backend/x86_subsystem_rebuild_plan.md`
explains the current subsystem shape and the replacement layout, explicitly
states how the stage-1 extraction set must be corrected before later stages
rely on it, and `docs/backend/x86_subsystem_rebuild_handoff.md` exists as a
concrete intake contract for stage 3.
