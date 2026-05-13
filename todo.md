Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1f
Current Step Title: Extract `peephole.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.1f: Extract `peephole.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/codegen/peephole.cpp` into
`src/backend/mir/aarch64/codegen/peephole.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old text-first AArch64 peephole contract,
including line classification, tombstone rendering, iterative local pass
ordering, copy propagation, dead stack-store elimination, the disabled global
store-forwarding note, shared string-helper dependencies, hidden assumptions,
tests preserved in the old surface, and rebuild risks.

## Suggested Next

Next coherent packet: stay within Step 2.1 and extract
`src/backend/mir/aarch64/codegen/mod.cpp` to a markdown artifact, then remove
that old `.cpp` from the live tree.

Continue the remaining codegen extraction lane through these old codegen
surfaces before moving into assembler, encoder, linker, or module-entry files:

- `mod.cpp`

After Step 2.1, continue Step 2 through these bounded lanes:

- Step 2.2: assembler parser/writer surfaces:
  `assembler/parser.cpp`, `assembler/elf_writer.cpp`, and
  `assembler/mod.cpp`.
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

- `peephole.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived peephole surface has several rebuild hazards: disabled global
  store forwarding was tied to a known complex float-array correctness bug,
  `mov wN, wN` must not be treated as a no-op, copy propagation needs
  whole-word register replacement, and dead-store elimination must bail out
  when the stack address escapes.
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
