Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.2a
Current Step Title: Extract assembler `parser.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.2a: Extract assembler `parser.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/assembler/parser.cpp` into
`src/backend/mir/aarch64/assembler/parser.md` and removed the old `.cpp` from
the live tree.

The markdown artifact records the old AArch64 assembler parser surface,
including the `parse_asm` and `trim_asm` entry points, statement
classification rules, operand splitting behavior, unused directive/data shape
types, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Next coherent packet: continue Step 2.2 by extracting
`src/backend/mir/aarch64/assembler/elf_writer.cpp` to markdown and removing
that old `.cpp` from the live tree, unless the supervisor chooses to extract
the assembler module surface first.

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

- Step 2.2 has started with the assembler parser surface; continue within the
  parser/writer lane rather than widening into encoder, linker, or top-level
  module-entry files.
- `parser.md` documents a line-oriented legacy parser. Treat it as a historical
  contract for statement normalization, not as a complete AArch64 grammar.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.
- Treat the Step 2 lane labels above as execution-state substeps, not durable
  source-idea changes. If a lane exposes a missing BIR/prepared carrier or a
  separate implementation initiative, record that as a separate lifecycle item
  instead of expanding Step 2.
- The retargeted signature metadata test is artifact coverage only; do not
  count it as live AArch64 backend behavior proof.
- `parser.hpp` remains in the tree and still declares the historical parser
  entry points; deleting or replacing headers is outside this packet.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
