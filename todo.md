Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1c
Current Step Title: Extract `intrinsics.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.1c: Extract `intrinsics.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/codegen/intrinsics.cpp` into
`src/backend/mir/aarch64/codegen/intrinsics.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old intrinsic lowering contract, including
barrier/cache mappings, non-temporal and unaligned vector memory shapes, NEON
binary operations, `Pmovmskb128`, CRC32 builtins, frame/return/thread address
builtins, scalar FP unary operations, x86-only fallback behavior, dependencies,
hidden assumptions, and rebuild risks.

## Suggested Next

Next coherent packet: stay within Step 2.1 and extract
`src/backend/mir/aarch64/codegen/i128_ops.cpp` to a markdown artifact, then
remove that old `.cpp` from the live tree.

Continue the remaining codegen extraction lane through these old codegen
surfaces before moving into assembler, encoder, linker, or module-entry files:

- `i128_ops.cpp`
- `f128.cpp`
- `peephole.cpp`
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

- `intrinsics.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived intrinsics surface has several rebuild hazards: 128-bit vector
  operands are pointer-carried, scalar FP values are raw bits in integer
  registers until `fmov`, `Pcmpeqd128` needs `.4s` lane comparisons,
  `Pmovmskb128` is lane-order-sensitive, CRC instructions need feature-policy
  attention, and many x86-only intrinsics currently zero-fill rather than trap.
- The old file only documented the F128 soft-float helper boundary; do not
  treat this artifact as complete binary128 lowering coverage.
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
