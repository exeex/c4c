# Review Extracted X86 Codegen Subsystem For Phoenix Rebuild

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md)

## Intent

Review the extracted markdown model of `src/backend/mir/x86/codegen/`,
reconstruct how the current subsystem actually works, decide how the stage-1
extraction set itself must be improved, and define the replacement
architecture layout before any replacement draft is written.

## Stage In Sequence

Stage 2 of 4: extraction review and replacement layout.

## Produces

- `docs/backend/x86_codegen_rebuild_plan.md`
- `docs/backend/x86_codegen_rebuild_handoff.md`

The review and layout artifact must:

- broadly review the full extraction set under `docs/backend/x86_codegen_legacy/`
- reconstruct how the current design actually routes ownership, dispatch, and
  hidden dependencies across the subsystem
- judge whether the stage-1 extraction artifacts are actually shaped well
  enough for redesign work, instead of treating them as automatically
  sufficient
- identify where the extraction set needs correction, expansion, compression,
  reclassification, or reorganization before stage 3 should trust it
- identify what responsibilities are mixed together and what interfaces are
  falsely coupled
- identify which APIs and contracts are truly stable enough to preserve
- define a concrete replacement architecture layout for the rebuilt subsystem
- explicitly judge whether that layout addresses the historical failure
  pressure that surfaced in idea 75 and the prepared-route runtime/call-lane
  family behind it
- specify the exact replacement draft-file layout that stage 3 must produce,
  including every planned `.cpp.md`, every planned `.hpp.md`, and any
  directory-level index markdown the draft stage will need
- write an explicit stage-2-to-stage-3 handoff that tells the draft stage
  which extraction artifacts must be trusted as-is, which must be corrected
  first, which replacement files are mandatory, and what route constraints the
  draft stage must preserve

## Does Not Yet Own

This stage does not own:

- writing replacement `.cpp.md` / `.hpp.md` contents
- converting drafts into real implementation
- deleting legacy code

## Unlocks

This stage unlocks stage 3 by turning the extracted legacy artifact set into a
reviewed and, when necessary, corrected redesign input plus an explicit
replacement layout, draft manifest, and handoff packet instead of leaving the
next stage to inherit a weak extraction set or invent file boundaries
opportunistically.

## Scope Notes

The review must critique the whole subsystem shape, including:

- whether the idea-78 per-file `.md` set captures the right APIs, contracts,
  dependency directions, hidden state, and special-case labels at the right
  level of compression
- which extraction artifacts should be rewritten, split, merged, expanded, or
  simplified before the rebuild should trust them as design evidence
- whether the directory-level extraction index is telling the truth about the
  subsystem's actual ownership map
- where `prepared_*.cpp` behaves like a parallel lowering stack instead of a
  consumer of canonical seams
- where dispatch, lowering, addressing, and emission responsibilities are
  coupled more tightly than they should be
- what should survive into the redesign, what should be isolated as
  compatibility, and what should be rejected as overfit

## Boundaries

Do not draft replacement file contents yet. Stage 2 owns review, diagnosis,
extraction-set improvement guidance, replacement layout, and the exact stage-3
draft manifest plus handoff only.

## Completion Signal

This idea is complete when `docs/backend/x86_codegen_rebuild_plan.md` explains
how the current subsystem really works, critiques the false seams and stable
seams, explicitly states how the idea-78 extraction set must be improved
before later stages rely on it, defines a concrete replacement layout,
explicitly judges whether that layout addresses the motivating prepared-route
failure family, names the complete stage-3 `.cpp.md` / `.hpp.md` artifact set
that must exist next, and `docs/backend/x86_codegen_rebuild_handoff.md`
provides an explicit contract for what stage 3 must consume and preserve.
