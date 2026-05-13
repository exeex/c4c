Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1
Current Step Title: Extract Remaining Codegen `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2.1: Extract Remaining Codegen `.cpp` Surfaces To Markdown Artifacts
extracted `src/backend/mir/aarch64/codegen/alu.cpp` into
`src/backend/mir/aarch64/codegen/alu.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old ALU lowering contract, including
integer and floating unary operations, CLZ/CTZ/byte-swap/popcount sequences,
unsigned power-of-two division and remainder reductions, register-direct binary
operation fast paths, accumulator fallback behavior, 32-bit sign/zero-extension
rules, i128 copy behavior, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Current execution lane: Step 2.1, remaining codegen `.cpp` extraction.

Extract the remaining old codegen surfaces under
`src/backend/mir/aarch64/codegen/` before moving into assembler, encoder,
linker, or module-entry files:

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

- `alu.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: signed 32-bit operations
  need explicit `sxtw`, unsigned 32-bit operations rely on `w`-register
  zero-extension, power-of-two reductions are unsigned-only, register-direct
  ALU must avoid right-operand destination-register clobbering, 16-bit
  byte-swap needs the post-`rev` shift, and i128 copy must preserve both
  `x0` and `x1`.
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
