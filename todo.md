Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.3b
Current Step Title: Extract encoder `compare_branch.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.3b: Extract encoder `compare_branch.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/assembler/encoder/compare_branch.cpp` into
`src/backend/mir/aarch64/assembler/encoder/compare_branch.md` and removed the old
`.cpp` from the live tree.

The markdown artifact records the old AArch64 compare/branch encoder surface,
including compare aliases, conditional compare, conditional select aliases,
direct and symbolic branch forms, compare-and-branch forms, test-and-branch
forms, relocation dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Next coherent packet: continue Step 2.3 by extracting
`src/backend/mir/aarch64/assembler/encoder/data_processing.cpp` to markdown and
removing that old `.cpp` from the live tree.

Step 2.3 covers these assembler encoder surfaces:
`assembler/encoder/compare_branch.cpp`,
`assembler/encoder/data_processing.cpp`,
`assembler/encoder/fp_scalar.cpp`,
`assembler/encoder/load_store.cpp`, `assembler/encoder/neon.cpp`,
`assembler/encoder/system.cpp`, and `assembler/encoder/mod.cpp`.

After Step 2.3, continue Step 2 through these bounded lanes:
- Step 2.4: linker surfaces:
  `linker/elf.cpp`, `linker/emit_dynamic.cpp`, `linker/emit_shared.cpp`,
  `linker/emit_static.cpp`, `linker/input.cpp`, `linker/link.cpp`,
  `linker/plt_got.cpp`, `linker/reloc.cpp`, `linker/types.cpp`, and
  `linker/mod.cpp`.
- Step 2.5: top-level module entry surface: `mod.cpp`.

## Watchouts

- Step 2.3 now has the bitfield and compare/branch encoder artifacts; do not
  revisit those removed `.cpp` files unless the supervisor opens a new packet.
- `compare_branch.md` documents the historical commented encoder formulas only.
  Treat its alias mappings and relocation notes as rebuild guidance, not as
  proof that the built-in AArch64 assembler path is production-ready.
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
- Step 2.3 encoder extraction should stay descriptive. Do not repair encoder
  coverage, relocation behavior, or instruction semantics during the markdown
  extraction packets.
- `data_processing.cpp` is the next encoder-family surface. Keep it separate
  from the compare/branch artifact even where aliases delegate into data
  processing helpers.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
