# Full X86 Subsystem Legacy Extraction Index

This directory is the stage-1 Phoenix extraction package for the full
`src/backend/mir/x86/` subsystem.

It extends the already-accepted codegen extraction set under
`docs/backend/x86_codegen_legacy/` so the root dispatcher, assembler, linker,
and codegen tree can be reviewed as one ownership graph before more live-tree
teardown proceeds.

## Boundary

Accepted prior evidence:

- codegen subtree: [docs/backend/x86_codegen_legacy/index.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/index.md)
- reviewed codegen replacement layout: [docs/backend/x86_codegen_rebuild/index.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild/index.md)
- codegen replacement handoff: [docs/backend/x86_codegen_rebuild_handoff.md](/workspaces/c4c/docs/backend/x86_codegen_rebuild_handoff.md)

New stage-1 extraction scope for this whole-subsystem route:

- `src/backend/mir/x86/mod.cpp`
- `src/backend/mir/x86/assembler/mod.hpp`
- `src/backend/mir/x86/assembler/mod.cpp`
- `src/backend/mir/x86/assembler/elf_writer.cpp`
- `src/backend/mir/x86/assembler/parser.cpp`
- `src/backend/mir/x86/assembler/encoder/mod.hpp`
- `src/backend/mir/x86/assembler/encoder/avx.cpp`
- `src/backend/mir/x86/assembler/encoder/core.cpp`
- `src/backend/mir/x86/assembler/encoder/gp_integer.cpp`
- `src/backend/mir/x86/assembler/encoder/mod.cpp`
- `src/backend/mir/x86/assembler/encoder/registers.cpp`
- `src/backend/mir/x86/assembler/encoder/sse.cpp`
- `src/backend/mir/x86/assembler/encoder/system.cpp`
- `src/backend/mir/x86/assembler/encoder/x87_misc.cpp`
- `src/backend/mir/x86/linker/mod.hpp`
- `src/backend/mir/x86/linker/mod.cpp`
- `src/backend/mir/x86/linker/elf.cpp`
- `src/backend/mir/x86/linker/emit_exec.cpp`
- `src/backend/mir/x86/linker/emit_shared.cpp`
- `src/backend/mir/x86/linker/input.cpp`
- `src/backend/mir/x86/linker/link.cpp`
- `src/backend/mir/x86/linker/plt_got.cpp`
- `src/backend/mir/x86/linker/types.cpp`

## Directory Index Ownership

Accepted non-helper directory index headers for the Phoenix route:

- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/mir/x86/assembler/mod.hpp`
- `src/backend/mir/x86/assembler/encoder/mod.hpp`
- `src/backend/mir/x86/linker/mod.hpp`

Compatibility or helper-only headers that must not be treated as second
directory indexes:

- `src/backend/mir/x86/assembler/parser.hpp`

## Stage-1 Artifact Map

Already satisfied by accepted prior Phoenix evidence:

- `codegen/` per-file companions remain under
  [docs/backend/x86_codegen_legacy/index.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/index.md)

Still required under `docs/backend/x86_subsystem_legacy/`:

- `mod.cpp.md`
- `assembler/mod.hpp.md`
- `assembler/mod.cpp.md`
- `assembler/elf_writer.cpp.md`
- `assembler/parser.cpp.md`
- `assembler/encoder/mod.hpp.md`
- `assembler/encoder/avx.cpp.md`
- `assembler/encoder/core.cpp.md`
- `assembler/encoder/gp_integer.cpp.md`
- `assembler/encoder/mod.cpp.md`
- `assembler/encoder/registers.cpp.md`
- `assembler/encoder/sse.cpp.md`
- `assembler/encoder/system.cpp.md`
- `assembler/encoder/x87_misc.cpp.md`
- `linker/mod.hpp.md`
- `linker/mod.cpp.md`
- `linker/elf.cpp.md`
- `linker/emit_exec.cpp.md`
- `linker/emit_shared.cpp.md`
- `linker/input.cpp.md`
- `linker/link.cpp.md`
- `linker/plt_got.cpp.md`
- `linker/types.cpp.md`

## Current Teardown State

The live tree currently has these in-scope legacy files deleted and therefore
blocked on matching extraction evidence before that deletion can count as
Phoenix progress:

- `src/backend/mir/x86/mod.cpp`
- `src/backend/mir/x86/assembler/mod.cpp`
- `src/backend/mir/x86/assembler/elf_writer.cpp`
- `src/backend/mir/x86/assembler/parser.cpp`
- `src/backend/mir/x86/assembler/encoder/mod.cpp`
- `src/backend/mir/x86/assembler/encoder/avx.cpp`
- `src/backend/mir/x86/assembler/encoder/core.cpp`
- `src/backend/mir/x86/assembler/encoder/gp_integer.cpp`
- `src/backend/mir/x86/assembler/encoder/registers.cpp`
- `src/backend/mir/x86/assembler/encoder/sse.cpp`
- `src/backend/mir/x86/assembler/encoder/system.cpp`
- `src/backend/mir/x86/assembler/encoder/x87_misc.cpp`
- `src/backend/mir/x86/linker/mod.cpp`
- `src/backend/mir/x86/linker/elf.cpp`
- `src/backend/mir/x86/linker/emit_exec.cpp`
- `src/backend/mir/x86/linker/emit_shared.cpp`
- `src/backend/mir/x86/linker/input.cpp`
- `src/backend/mir/x86/linker/link.cpp`
- `src/backend/mir/x86/linker/plt_got.cpp`
- `src/backend/mir/x86/linker/types.cpp`

The accepted next packet is to create the missing per-file extraction artifacts
from legacy evidence, then re-check whether the current `.cpp` deletions may
stay removed or must be restored.
