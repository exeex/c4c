# Extract Full X86 Backend Subsystem To Markdown For Phoenix Rebuild

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Parent Idea: [81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md)

## Intent

Broaden the Phoenix rebuild from `src/backend/mir/x86/codegen/` to the full
`src/backend/mir/x86/` subsystem so the root dispatcher, assembler, linker,
and existing codegen rebuild all live under one explicit staged teardown and
replacement contract.

## Stage In Sequence

Stage 1 of 4: extraction.

## Produces

- `docs/backend/x86_subsystem_legacy/index.md`
- `docs/backend/x86_subsystem_legacy/mod.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/mod.hpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/mod.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/elf_writer.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/parser.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/mod.hpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/avx.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/core.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/gp_integer.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/mod.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/registers.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/sse.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/system.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/x87_misc.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/mod.hpp.md`
- `docs/backend/x86_subsystem_legacy/linker/mod.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/elf.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/emit_exec.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/emit_shared.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/input.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/link.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/plt_got.cpp.md`
- `docs/backend/x86_subsystem_legacy/linker/types.cpp.md`
- an explicit top-level coverage note that the existing
  `docs/backend/x86_codegen_legacy/` package remains the accepted per-file
  extraction set for `src/backend/mir/x86/codegen/`, including the single
  directory-index header `x86_codegen.hpp`

The extraction artifacts must:

- use `.codex/skills/phoenix-rebuild/SKILL.md` as the stage-1 extraction rule
  before any Phoenix executor or reviewer continues
- keep one markdown companion for every in-scope legacy `.cpp`
- keep one markdown companion for the single accepted non-helper directory
  index `.hpp` in each in-scope directory: `assembler/mod.hpp`,
  `assembler/encoder/mod.hpp`, `linker/mod.hpp`, and the already-extracted
  `codegen/x86_codegen.hpp`
- keep `parser.hpp` and any other non-index header content documented as local
  helper or compatibility surface instead of treating it as a second directory
  index
- compress important APIs, contracts, dependency directions, hidden
  dependencies, responsibility buckets, and special-case classifications
- use short fenced `cpp` blocks only for essential representative surfaces

## Does Not Yet Own

This stage does not own:

- deciding the replacement architecture for the full subsystem
- drafting replacement `.cpp.md` / `.hpp.md` files
- converting drafts into real implementation
- accepting permanent legacy deletion before matching extraction evidence
  exists

## Unlocks

This stage unlocks one honest review of the full root-dispatch, assembler,
linker, and codegen ownership graph instead of letting codegen-only Phoenix
artifacts stand in for the rest of `src/backend/mir/x86/`.

## Scope Notes

- Read `.codex/skills/phoenix-rebuild/SKILL.md` first before doing stage-1
  Phoenix work on this idea.
- Treat the existing `docs/backend/x86_codegen_legacy/` package as accepted
  prior evidence for the codegen subtree; do not regenerate it unless stage 2
  later proves it is insufficient.
- Treat current worktree deletions under `src/backend/mir/x86/assembler/`,
  `src/backend/mir/x86/linker/`, and `src/backend/mir/x86/mod.cpp` as teardown
  intent that still requires matching stage-1 extraction evidence before the
  deletion can count as Phoenix progress.
- Classify every notable fast path or special case as `core lowering`,
  `optional fast path`, `legacy compatibility`, or `overfit to reject`.

## Boundaries

This idea does not authorize redesign or implementation conversion. It exists
to create the full evidence package needed before the whole `x86` subsystem is
split, deleted, or rewired further.

## Completion Signal

This idea is complete when every in-scope legacy `src/backend/mir/x86/*.cpp`,
`src/backend/mir/x86/assembler/**/*.cpp`, and
`src/backend/mir/x86/linker/*.cpp` file has its corresponding companion
markdown artifact under `docs/backend/x86_subsystem_legacy/`, the accepted
directory-index header companions exist for `assembler/mod.hpp`,
`assembler/encoder/mod.hpp`, `linker/mod.hpp`, and the already-extracted
`codegen/x86_codegen.hpp`, the top-level
`docs/backend/x86_subsystem_legacy/index.md` points at the full artifact set,
the extraction remains compressed enough to review, and any in-scope legacy
`.cpp` deleted from the live tree has either been removed through the Phoenix
cleanup rule after extraction or has been explicitly restored before later
stages proceed.
