# AArch64 Module Phoenix Stage 3 Replacement Drafts

Status: Open
Created: 2026-05-14
Parent Context: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Requires: ideas/closed/226_aarch64_module_phoenix_review_replacement_layout.md

## Intent

Draft the reviewed replacement module/MIR lowering components as markdown
design artifacts before any real implementation conversion begins.

The draft set must describe how prepared BIR lowers directly to MIR machine
nodes through clean ownership seams instead of reviving the legacy
`module.cpp` record-emitter shape.

Read this first before doing phoenix-stage work:
`.codex/skills/phoenix-rebuild/SKILL.md`.

## Stage In Sequence

Stage 3 of 4: replacement `.cpp.md` / `.hpp.md` draft generation and draft
review.

## Produces

- One `.cpp.md` for every planned replacement implementation file declared by
  stage 2.
- One directory-index non-helper `.hpp.md` for each replacement directory
  declared by stage 2.
- One replacement directory-level index `.md` when required by the stage-2
  layout.
- One review artifact for the replacement draft set.

## Does Not Yet Own

- Real `.cpp` / `.hpp` implementation.
- Build-system rewiring, dispatcher changes, or deletion of live compatibility
  code.
- Changes to tests or expectations.
- New component files not first declared by the stage-2 layout or a stage-2
  contract repair.

## Unlocks

Stage 4, which converts the reviewed markdown drafts into real implementation
through staged migration and proof.

## Scope Notes

- Follow the exact replacement file layout declared by stage 2.
- Follow the stage-2-to-stage-3 handoff contract.
- Partition by responsibility and dependency direction, not by arbitrary
  slices of legacy source.
- Each draft file must state owned responsibility, owned inputs, owned
  outputs, indirect queries, forbidden knowledge, and whether it is core
  lowering, dispatch, optional fast path, or compatibility.
- Expected seam families may include layout or addressing resolution,
  value-home or operand resolution, instruction lowering, branch lowering,
  call lowering, function or module dispatch, and optional pattern fast paths,
  but the final set is governed by the stage-2 layout.
- If stage 3 needs to add or remove planned files, repair the stage-2 contract
  first instead of silently drifting.

## Boundaries

- Do not collapse multiple planned replacement files into one catch-all draft.
- Do not require two components to know each other's full internal context.
- Do not preserve cached display spelling or flat target-local vectors as the
  primary MIR carrier unless explicitly isolated as compatibility.
- Do not convert drafts into real source in this stage.

## Completion Signal

This idea is complete only when every replacement `.cpp` and every
directory-index non-helper `.hpp` declared by stage 2 has a corresponding
`.cpp.md` / `.hpp.md`, no replacement directory introduces more than one
non-helper `.hpp` except for allowed `helper.hpp`, any required replacement
index `.md` exists, each draft states its ownership contract, and the
draft-review artifact says the draft set is coherent enough for implementation
conversion.

## Reviewer Reject Signals

Reject the route if drafts ignore the stage-2 handoff, invent or remove files
without a stage-2 repair, split by legacy line ranges instead of ownership,
retain the old module emitter behind new names, depend on cached display
strings as the main lowering contract, claim capability progress through test
rewrites, or hide unresolved two-way coupling between replacement components.
