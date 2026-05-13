Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1
Current Step Title: Extract Remaining Codegen `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2.1: Extract Remaining Codegen `.cpp` Surfaces To Markdown Artifacts
extracted `src/backend/mir/aarch64/codegen/variadic.cpp` into
`src/backend/mir/aarch64/codegen/variadic.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old AAPCS64 variadic ABI contract, including
`va_start`, scalar and aggregate `va_arg`, `va_copy`, `va_list` field offsets,
GP and FP/SIMD register-save offsets, stack fallback, F128 helper conversion,
dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Current execution lane: Step 2.1, remaining codegen `.cpp` extraction.

Extract the remaining old codegen surfaces under
`src/backend/mir/aarch64/codegen/` before moving into assembler, encoder,
linker, or module-entry files:

- `inline_asm.cpp`
- `alu.cpp`
- `memory.cpp`
- `calls.cpp`
- `cast_ops.cpp`
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

- `variadic.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: signed `__gr_offs` and
  `__vr_offs` branch semantics, 8-byte GP slots versus 16-byte FP/SIMD slots,
  whole-aggregate register-or-stack selection, stack fallback alignment, F128
  `__trunctfdf2` conversion, and cache invalidation after helper/aggregate
  paths.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.
- Treat the Step 2 lane labels above as execution-state substeps, not durable
  source-idea changes. If a lane exposes a missing BIR/prepared carrier or a
  separate implementation initiative, record that as a separate lifecycle item
  instead of expanding Step 2.
- The retargeted signature metadata test is artifact coverage only; do not
  count it as live AArch64 backend behavior proof.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
