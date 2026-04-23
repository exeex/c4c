# Convert Reviewed Full X86 Backend Drafts To Implementation For Phoenix Rebuild

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Parent Idea: [84_draft_replacement_full_x86_backend_interfaces_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/84_draft_replacement_full_x86_backend_interfaces_for_phoenix_rebuild.md)

## Intent

Convert the reviewed replacement drafts for the full `src/backend/mir/x86/`
subsystem into real staged implementation.

## Stage In Sequence

Stage 4 of 4: draft-to-implementation conversion.

## Produces

- real `.cpp` / `.hpp` implementation under `src/backend/mir/x86/` that
  matches the reviewed draft set from `docs/backend/x86_subsystem_rebuild/`
- dispatcher and ownership rewiring that routes root dispatch, assembler,
  linker, and codegen behavior through the reviewed replacement seams
- legacy deletion or retirement only after the new owners are live and proved

## Does Not Yet Own

This stage does not own:

- changing the reviewed draft contract without first repairing stage 3
- deleting live legacy code before the new owner is actually in use and proved
- redefining the replacement architecture outside the reviewed draft set

## Unlocks

This stage unlocks eventual retirement of the old `src/backend/mir/x86/`
subsystem shape once the whole ownership graph is live, proven, and no longer
depends on compatibility stubs.

## Scope Notes

- Read `.codex/skills/phoenix-rebuild/SKILL.md` first before doing stage-4
  Phoenix work on this idea.
- Use staged migration rather than an all-at-once rewrite.
- Every packet must state what responsibility moved, what remains legacy, and
  what proof covers the migrated seam.
- Preserve accepted earlier Phoenix work for `codegen/` where it still matches
  the reviewed full-subsystem draft set, but do not let those older seams
  bypass the new top-level ownership contract.

## Boundaries

Keep the old route available until the reviewed replacement owners actually
own the migrated behavior.

## Completion Signal

This idea is complete when the reviewed full-subsystem draft set has been
converted into real `src/backend/mir/x86/` implementation, the new ownership
seams are actually in use, remaining legacy code is explicitly classified, and
proof shows the migrated capability families still work.
