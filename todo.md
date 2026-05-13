Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.4c
Current Step Title: Extract linker `emit_shared.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.4c: Extract linker `emit_shared.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/linker/emit_shared.cpp` into
`src/backend/mir/aarch64/linker/emit_shared.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old AArch64 shared-library emission surface,
including the `emit_shared_library` entry point, dynamic symbol/string table
construction, GNU hash undefined/export partitioning, ELF/program-header and
section-header layout, PLT/GOT and `.dynamic` emission, relocation behavior,
linker-provided symbols, dependencies, assumptions, and rebuild risks.

## Suggested Next

Next coherent packet: continue Step 2.4 by extracting
`src/backend/mir/aarch64/linker/emit_static.cpp` to markdown and removing that
old `.cpp` from the live tree.

Step 2.3 now has markdown artifacts for these assembler encoder surfaces:
`assembler/encoder/compare_branch.cpp`,
`assembler/encoder/data_processing.cpp`,
`assembler/encoder/fp_scalar.cpp`,
`assembler/encoder/load_store.cpp`, `assembler/encoder/neon.cpp`,
`assembler/encoder/system.cpp`, and `assembler/encoder/mod.cpp`.

After Step 2.3, continue Step 2 through these bounded lanes:
- Step 2.4: remaining linker surfaces:
  `linker/emit_static.cpp`, `linker/input.cpp`, `linker/link.cpp`,
  `linker/plt_got.cpp`, `linker/reloc.cpp`, `linker/types.cpp`, and
  `linker/mod.cpp`.
- Step 2.5: top-level module entry surface: `mod.cpp`.

## Watchouts

- Step 2.4 now has the linker ELF facade, dynamic executable emission, and
  shared-library emission artifacts; do not revisit the removed
  `linker/elf.cpp`, `linker/emit_dynamic.cpp`, or `linker/emit_shared.cpp`
  unless the supervisor opens a new packet.
- `linker/emit_dynamic.md` documents the old dynamic executable emitter,
  including PLT/GOT, `.dynamic`, `.gnu.hash`, dynamic relocations, copy
  relocations, and TLS layout. Treat it as rebuild guidance, not as proof that
  the live C++ backend can emit AArch64 dynamic executables.
- `linker/emit_shared.md` documents the old shared-object emitter, including
  `ET_DYN` output, exported/imported dynsym ordering, `.gnu.hash`, PLT/GOT,
  `.rela.dyn`, `.rela.plt`, SONAME/NEEDED tags, init/fini arrays, TLS, and
  section headers. Treat it as rebuild guidance, not as proof that the live
  C++ backend can emit AArch64 shared libraries.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.
- Treat the Step 2 lane labels above as execution-state substeps, not durable
  source-idea changes. If a lane exposes a missing BIR/prepared carrier or a
  separate implementation initiative, record that as a separate lifecycle item
  instead of expanding Step 2.
- The retargeted signature metadata test is artifact coverage only; do not
  count it as live AArch64 backend behavior proof.
- `mod.hpp`, `parser.hpp`, and other headers remain in the tree; deleting or
  replacing headers is outside this packet.
- Step 2.4 linker extraction should stay descriptive. Do not repair linker,
  relocation, dynamic emission, or ELF parsing behavior during the markdown
  extraction packets.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
