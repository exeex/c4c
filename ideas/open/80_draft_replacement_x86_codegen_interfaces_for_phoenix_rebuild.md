# Draft Replacement X86 Codegen Interfaces For Phoenix Rebuild

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md)

## Intent

Draft the replacement x86 codegen subsystem as reviewed markdown interfaces
before any implementation conversion begins.

## Stage In Sequence

Stage 3 of 4: replacement drafting.

## Produces

- `docs/backend/x86_codegen_rebuild.hpp.md`
- `docs/backend/x86_codegen_rebuild.cpp.md`
- `docs/backend/x86_codegen_rebuild_review.md`

The drafts must define:

- the replacement component graph for `src/backend/mir/x86/codegen/`
- the dispatcher boundaries that decide which `.cpp` owns which lowering family
- how prepared routes consume shared seams instead of growing a parallel
  lowering stack
- which responsibilities are core lowering, dispatch, optional fast path, or
  legacy compatibility

## Does Not Yet Own

This stage does not own:

- real `.cpp` / `.hpp` implementation edits
- deleting the old subsystem
- proving final runtime correctness

## Unlocks

This stage unlocks stage 4 conversion by giving implementation work a reviewed
replacement contract and ownership map.

## Scope Notes

The draft must partition behavior by responsibility and dependency direction,
not by arbitrary line ranges or one-file-per-old-file mirroring.

## Boundaries

Do not convert drafts into real source during this stage.

## Completion Signal

This idea is complete when the reviewed `.hpp.md` / `.cpp.md` drafts define a
clean ownership model for the replacement x86 codegen subsystem and explain how
prepared paths reuse canonical seams instead of bypassing them.
