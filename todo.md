Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1a
Current Step Title: Extract `cast_ops.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.1a: Extract `cast_ops.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/codegen/cast_ops.cpp` into
`src/backend/mir/aarch64/codegen/cast_ops.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old scalar cast-lowering contract, including
the shared `CastKind` classification routes, integer accumulator convention,
FP/SIMD register-file bridges, width normalization rules, F128 soft-float
delegation, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Next coherent packet: stay within Step 2.1 and extract
`src/backend/mir/aarch64/codegen/atomics.cpp` to a markdown artifact, then
remove that old `.cpp` from the live tree.

Continue the remaining codegen extraction lane through these old codegen
surfaces before moving into assembler, encoder, linker, or module-entry files:

- `atomics.cpp`
- `intrinsics.cpp`
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

- `cast_ops.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived cast surface has several rebuild hazards: scalar floating casts
  require `fmov` bridges between the integer accumulator and FP/SIMD registers;
  signed narrow and signed 32-bit results need explicit sign-extension;
  unsigned narrow and unsigned 32-bit results need masking or `mov w0, w0`
  normalization; signed-to-float must extend narrow signed sources before
  conversion; and F128 casts must stay on the soft-float route rather than this
  scalar instruction body.
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
