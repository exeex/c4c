# Draft Replacement Full X86 Backend Interfaces For Phoenix Rebuild

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Parent Idea: [83_review_extracted_full_x86_backend_subsystem_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/83_review_extracted_full_x86_backend_subsystem_for_phoenix_rebuild.md)

## Intent

Draft the reviewed replacement file set for the full `src/backend/mir/x86/`
subsystem before converting it into live implementation.

## Stage In Sequence

Stage 3 of 4: replacement draft generation and draft review.

## Produces

- one `.cpp.md` for every planned replacement implementation file declared by
  `docs/backend/x86_subsystem_rebuild_plan.md`
- one directory-index non-helper `.hpp.md` for each replacement directory
  declared by the stage-2 layout
- any needed replacement directory-level index `.md`
- `docs/backend/x86_subsystem_rebuild/review.md`

## Does Not Yet Own

This stage does not own:

- changing the reviewed stage-2 replacement layout without first repairing
  stage 2
- converting the draft set into real implementation
- deleting legacy live code

## Unlocks

This stage unlocks implementation conversion once the draft set is coherent
enough to review as the future whole-`x86` ownership graph.

## Scope Notes

- Read `.codex/skills/phoenix-rebuild/SKILL.md` first before doing stage-3
  Phoenix work on this idea.
- Follow the exact replacement file layout and handoff contract from
  `docs/backend/x86_subsystem_rebuild_plan.md` and
  `docs/backend/x86_subsystem_rebuild_handoff.md`.
- Partition by responsibility and dependency direction, not by copying old
  file boundaries mechanically.
- Each draft file should state owned inputs, owned outputs, indirect queries,
  forbidden knowledge, and whether it is `core logic`, `dispatch`, `optional
  fast path`, or `compatibility`.

## Boundaries

Do not convert markdown drafts into `.cpp` / `.hpp` in this stage.

## Completion Signal

This idea is complete when every replacement `.cpp` and each replacement
directory-index non-helper `.hpp` declared by the stage-2 layout has its
corresponding `.cpp.md` / `.hpp.md`, any required replacement index `.md`
exists, no replacement directory introduces more than one non-helper `.hpp`,
and `docs/backend/x86_subsystem_rebuild/review.md` says the draft set is
coherent enough for implementation conversion.
