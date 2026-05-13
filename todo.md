Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Existing AArch64 Surface

# Current Packet

## Just Finished

Step 1: Inventory Existing AArch64 Surface completed the filesystem inventory
for `src/backend/mir/aarch64/`.

Inventory summary:

- Total files: 48 files under `src/backend/mir/aarch64/`.
- Existing markdown/support artifact: `BINARY_UTILS_CONTRACT.md`.
- Headers/interfaces: 6 `.hpp` files.
- Old production extraction targets: 41 `.cpp` files.

Responsibility groups:

- Top-level module entry:
  - `src/backend/mir/aarch64/mod.cpp`
- Codegen and backend entry:
  - `src/backend/mir/aarch64/codegen/emit.cpp`
  - `src/backend/mir/aarch64/codegen/emit.hpp`
  - `src/backend/mir/aarch64/codegen/mod.cpp`
- Codegen feature shards / target ABI candidates:
  - `src/backend/mir/aarch64/codegen/alu.cpp`
  - `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
  - `src/backend/mir/aarch64/codegen/atomics.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/cast_ops.cpp`
  - `src/backend/mir/aarch64/codegen/comparison.cpp`
  - `src/backend/mir/aarch64/codegen/f128.cpp`
  - `src/backend/mir/aarch64/codegen/float_ops.cpp`
  - `src/backend/mir/aarch64/codegen/globals.cpp`
  - `src/backend/mir/aarch64/codegen/i128_ops.cpp`
  - `src/backend/mir/aarch64/codegen/inline_asm.cpp`
  - `src/backend/mir/aarch64/codegen/intrinsics.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/peephole.cpp`
  - `src/backend/mir/aarch64/codegen/prologue.cpp`
  - `src/backend/mir/aarch64/codegen/returns.cpp`
  - `src/backend/mir/aarch64/codegen/variadic.cpp`
- Assembler public surface and parser:
  - `src/backend/mir/aarch64/assembler/mod.cpp`
  - `src/backend/mir/aarch64/assembler/mod.hpp`
  - `src/backend/mir/aarch64/assembler/parser.cpp`
  - `src/backend/mir/aarch64/assembler/parser.hpp`
  - `src/backend/mir/aarch64/assembler/types.hpp`
- Assembler ELF writer / binary-utils candidate:
  - `src/backend/mir/aarch64/assembler/elf_writer.cpp`
- Assembler instruction encoder:
  - `src/backend/mir/aarch64/assembler/encoder/bitfield.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/compare_branch.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/data_processing.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/fp_scalar.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/load_store.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/mod.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/mod.hpp`
  - `src/backend/mir/aarch64/assembler/encoder/neon.cpp`
  - `src/backend/mir/aarch64/assembler/encoder/system.cpp`
- Linker public surface:
  - `src/backend/mir/aarch64/linker/mod.cpp`
  - `src/backend/mir/aarch64/linker/mod.hpp`
- Linker ELF/object input, relocation, and output:
  - `src/backend/mir/aarch64/linker/elf.cpp`
  - `src/backend/mir/aarch64/linker/emit_dynamic.cpp`
  - `src/backend/mir/aarch64/linker/emit_shared.cpp`
  - `src/backend/mir/aarch64/linker/emit_static.cpp`
  - `src/backend/mir/aarch64/linker/input.cpp`
  - `src/backend/mir/aarch64/linker/link.cpp`
  - `src/backend/mir/aarch64/linker/plt_got.cpp`
  - `src/backend/mir/aarch64/linker/reloc.cpp`
  - `src/backend/mir/aarch64/linker/types.cpp`
- Existing binary-utils contract/reference:
  - `src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md`

Old production `.cpp` files that must be extracted to markdown in Step 2:

- `src/backend/mir/aarch64/mod.cpp`
- `src/backend/mir/aarch64/assembler/elf_writer.cpp`
- `src/backend/mir/aarch64/assembler/mod.cpp`
- `src/backend/mir/aarch64/assembler/parser.cpp`
- `src/backend/mir/aarch64/assembler/encoder/bitfield.cpp`
- `src/backend/mir/aarch64/assembler/encoder/compare_branch.cpp`
- `src/backend/mir/aarch64/assembler/encoder/data_processing.cpp`
- `src/backend/mir/aarch64/assembler/encoder/fp_scalar.cpp`
- `src/backend/mir/aarch64/assembler/encoder/load_store.cpp`
- `src/backend/mir/aarch64/assembler/encoder/mod.cpp`
- `src/backend/mir/aarch64/assembler/encoder/neon.cpp`
- `src/backend/mir/aarch64/assembler/encoder/system.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
- `src/backend/mir/aarch64/codegen/atomics.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.cpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/inline_asm.cpp`
- `src/backend/mir/aarch64/codegen/intrinsics.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/mod.cpp`
- `src/backend/mir/aarch64/codegen/peephole.cpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/mir/aarch64/linker/elf.cpp`
- `src/backend/mir/aarch64/linker/emit_dynamic.cpp`
- `src/backend/mir/aarch64/linker/emit_shared.cpp`
- `src/backend/mir/aarch64/linker/emit_static.cpp`
- `src/backend/mir/aarch64/linker/input.cpp`
- `src/backend/mir/aarch64/linker/link.cpp`
- `src/backend/mir/aarch64/linker/mod.cpp`
- `src/backend/mir/aarch64/linker/plt_got.cpp`
- `src/backend/mir/aarch64/linker/reloc.cpp`
- `src/backend/mir/aarch64/linker/types.cpp`

## Suggested Next

Delegate Step 2 to extract `src/backend/mir/aarch64/codegen/emit.cpp` first.
It is the largest and most central old production surface, contains the current
backend emission entry points, and should become the first markdown artifact
before smaller codegen shards are handled.

## Watchouts

- Do not debug, patch, or expand old AArch64 `.cpp` files before the markdown
  review and interface ledger are complete.
- Do not add rendered-name recovery or string-based semantic lookup fallback.
- If a required BIR/prepared carrier is missing, split that into a separate
  `ideas/open/` gap idea instead of working around it in target code.
- Header files and `BINARY_UTILS_CONTRACT.md` were inventoried as support or
  reference artifacts, not Step 2 removal targets unless a later extraction
  packet explicitly owns interface cleanup.
- The Step 2 extraction should stay descriptive: summarize entry points,
  assumptions, dependencies, and failure risks before deleting old `.cpp`.

## Proof

Inventory-only lifecycle/todo update. Per delegated proof contract, no build or
compile proof was required and no `test_after.log` was generated.
