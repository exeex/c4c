Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.4a
Current Step Title: Extract linker `elf.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.4a: Extract linker `elf.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/linker/elf.cpp` into
`src/backend/mir/aarch64/linker/elf.md` and removed the old `.cpp` from the
live tree.

The markdown artifact records the old AArch64 ELF linker facade, including the
shared ELF re-export surface, AArch64 relocation constants, shared ELF64 type
aliases, the `parse_object` entry point, dependencies, assumptions, and rebuild
risks.

## Suggested Next

Next coherent packet: continue Step 2.4 by extracting
`src/backend/mir/aarch64/linker/emit_dynamic.cpp` to markdown and removing that
old `.cpp` from the live tree.

Step 2.3 now has markdown artifacts for these assembler encoder surfaces:
`assembler/encoder/compare_branch.cpp`,
`assembler/encoder/data_processing.cpp`,
`assembler/encoder/fp_scalar.cpp`,
`assembler/encoder/load_store.cpp`, `assembler/encoder/neon.cpp`,
`assembler/encoder/system.cpp`, and `assembler/encoder/mod.cpp`.

After Step 2.3, continue Step 2 through these bounded lanes:
- Step 2.4: remaining linker surfaces:
  `linker/emit_dynamic.cpp`, `linker/emit_shared.cpp`,
  `linker/emit_static.cpp`, `linker/input.cpp`, `linker/link.cpp`,
  `linker/plt_got.cpp`, `linker/reloc.cpp`, `linker/types.cpp`, and
  `linker/mod.cpp`.
- Step 2.5: top-level module entry surface: `mod.cpp`.

## Watchouts

- Step 2.4 now has the linker ELF facade artifact; do not revisit the removed
  `linker/elf.cpp` unless the supervisor opens a new packet.
- `linker/elf.md` documents a facade over shared ELF64 parsing and AArch64
  relocation constants only. Treat it as rebuild guidance, not as proof that
  the live C++ backend has an implemented AArch64 ELF parser.
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
