# Review Extracted X86 Codegen Subsystem For Phoenix Rebuild

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md)

## Intent

Review the extracted markdown model of `src/backend/mir/x86/codegen/` and
reconstruct how the current design actually works before any replacement draft
is written.

## Stage In Sequence

Stage 2 of 4: review.

## Produces

- `docs/backend/x86_codegen_subsystem_review.md`

The review must critique:

- what responsibilities are mixed together
- what interfaces are falsely coupled
- which APIs are truly stable and worth preserving
- where `prepared*.cpp` bypasses or duplicates canonical x86 codegen seams
- what should survive, be downgraded to compatibility, or be rejected as
  overfit

## Does Not Yet Own

This stage does not own:

- drafting replacement `.cpp.md` / `.hpp.md`
- implementation migration
- deleting legacy code

## Unlocks

This stage unlocks stage 3 replacement drafting by producing a subsystem-level
critique grounded in the extracted artifact instead of in ad hoc file reading.

## Scope Notes

The review must reconstruct the real dependency and dispatch story across the
full x86 codegen directory, not just list complaints about file size.

## Boundaries

Do not design new files yet. This stage is review and reconstruction only.

## Completion Signal

This idea is complete when `docs/backend/x86_codegen_subsystem_review.md`
concretely explains the current subsystem shape, calls out the false seams and
stable seams, and gives a grounded critique that can drive a clean redesign.
