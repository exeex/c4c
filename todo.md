# Execution State

Status: Active
Source Idea Path: ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract The Missing Root, Assembler, And Linker Legacy Sources
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

- completed plan step `1 - Freeze The Whole-Subsystem Extraction Inventory` by
  writing `docs/backend/x86_subsystem_legacy/index.md`, fixing the accepted
  directory-index headers, and recording which deleted root/assembler/linker
  legacy files still need stage-1 extraction evidence

## Suggested Next

- start the missing per-file extraction set under
  `docs/backend/x86_subsystem_legacy/` for `mod.cpp`, `assembler/`, and
  `linker/`, using legacy evidence rather than accepting the current deletions
  as progress

## Watchouts

- `docs/backend/x86_codegen_legacy/` is accepted prior stage-1 evidence for the
  codegen subtree and should be indexed, not regenerated, unless later review
  proves it insufficient
- current deletions under `src/backend/mir/x86/assembler/`,
  `src/backend/mir/x86/linker/`, and `src/backend/mir/x86/mod.cpp` do not
  count as Phoenix progress until matching extraction artifacts exist
- `assembler/parser.hpp` is not an accepted second directory index header; its
  contract should be documented as helper or compatibility content instead
- the new top-level inventory already treats `docs/backend/x86_codegen_legacy/`
  as accepted evidence, so the next packet should not duplicate that subtree

## Proof

- inventory-only packet; no code proof run in this packet
