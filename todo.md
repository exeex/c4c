Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1g
Current Step Title: Extract codegen `mod.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.1g: Extract codegen `mod.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/codegen/mod.cpp` into
`src/backend/mir/aarch64/codegen/mod.md` and removed the old `.cpp` from the
live tree.

The markdown artifact records the old commented AArch64 codegen module surface,
including its reference-module role, shard list, historical visibility notes,
namespace-only dependency shape, hidden assumptions, and rebuild risks.

## Suggested Next

Next coherent packet: start Step 2.2 by extracting the assembler parser/writer
lane. A bounded first packet would archive
`src/backend/mir/aarch64/assembler/parser.cpp` to markdown and remove that old
`.cpp` from the live tree, unless the supervisor chooses a grouped
parser/writer packet.

Step 2.2 covers these assembler parser/writer surfaces:
`assembler/parser.cpp`, `assembler/elf_writer.cpp`, and
`assembler/mod.cpp`.

After Step 2.2, continue Step 2 through these bounded lanes:
- Step 2.3: assembler encoder surfaces:
  `assembler/encoder/bitfield.cpp`,
  `assembler/encoder/compare_branch.cpp`,
  `assembler/encoder/data_processing.cpp`,
  `assembler/encoder/fp_scalar.cpp`,
  `assembler/encoder/load_store.cpp`, `assembler/encoder/neon.cpp`,
  `assembler/encoder/system.cpp`, and `assembler/encoder/mod.cpp`.
- Step 2.4: linker surfaces:
  `linker/elf.cpp`, `linker/emit_dynamic.cpp`, `linker/emit_shared.cpp`,
  `linker/emit_static.cpp`, `linker/input.cpp`, `linker/link.cpp`,
  `linker/plt_got.cpp`, `linker/reloc.cpp`, `linker/types.cpp`, and
  `linker/mod.cpp`.
- Step 2.5: top-level module entry surface: `mod.cpp`.

## Watchouts

- Step 2.1's codegen extraction lane is now complete through the delegated
  shard list; continue with assembler parser/writer extraction rather than
  widening into encoder, linker, or top-level module-entry files.
- `codegen/mod.cpp` was only a commented module index and namespace stub. Treat
  `mod.md` as a shard map, not as a complete dependency graph or pass schedule.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.
- Treat the Step 2 lane labels above as execution-state substeps, not durable
  source-idea changes. If a lane exposes a missing BIR/prepared carrier or a
  separate implementation initiative, record that as a separate lifecycle item
  instead of expanding Step 2.
- The retargeted signature metadata test is artifact coverage only; do not
  count it as live AArch64 backend behavior proof.
- Keep each remaining Step 2.1 codegen shard as a bounded descriptive
  extraction packet unless the supervisor explicitly delegates a grouped
  packet.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
