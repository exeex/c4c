Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.1a
Current Step Title: Extract `cast_ops.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.1: Extract Remaining Codegen `.cpp` Surfaces To Markdown Artifacts
extracted `src/backend/mir/aarch64/codegen/calls.cpp` into
`src/backend/mir/aarch64/codegen/calls.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old call-lowering contract, including AArch64
ABI configuration, stack argument staging, register argument helper sequencing,
direct and indirect call emission, stack cleanup, I128 and F128 result storage,
softfloat helper interactions, dependencies, hidden assumptions, and rebuild
risks.

## Suggested Next

Current execution packet: Step 2.1a, extract
`src/backend/mir/aarch64/codegen/cast_ops.cpp` to a markdown artifact and
remove that old `.cpp` from the live tree.

Stay within Step 2.1, the remaining codegen extraction lane. After
`cast_ops.cpp`, continue the lane through these old codegen surfaces before
moving into assembler, encoder, linker, or module-entry files:

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

- `calls.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived calls surface has several rebuild hazards: the AArch64 ABI
  config must preserve eight GP and eight FP argument registers, F128-in-FP
  policy, I128 pair alignment, large-struct-by-reference behavior, and
  dedicated sret policy; source stack-slot loads need outgoing-stack adjustment
  when dynamic alloca is absent; I128 and F128 stack arguments require 16-byte
  alignment; indirect calls load a spilled function pointer from the outgoing
  stack area; and F128 result handling must preserve both full `q0` storage and
  the `__trunctfdf2` accumulator update.
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
