Status: Active
Source Idea Path: ideas/open/330_native_object_model_and_emission_api.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current Object And Backend Surfaces

# Current Packet

## Just Finished

Step 1 research completed: inspected the current target-local assembler,
linker, AArch64 structured-record, CMake, and reference ELF writer surfaces for
child 330.

Recommended shared API location:

- namespace: `c4c::backend::mir::object`
- files:
  - `src/backend/mir/object/model.hpp`
  - `src/backend/mir/object/model.cpp`
  - later writer slice: `src/backend/mir/object/elf_writer.hpp` and
    `src/backend/mir/object/elf_writer.cpp`
- build wiring: add the `.cpp` files to `src/backend/CMakeLists.txt` so both
  RV64 and AArch64 children can depend through `c4c_backend` without importing
  either target's assembler namespace.
- first tests:
  - `tests/backend/mir/backend_object_model_records_test.cpp`
  - CMake entry through existing `c4c_add_mir_cpp_test(...)` in
    `tests/backend/mir/CMakeLists.txt`

Existing helpers and gaps:

- `src/backend/mir/riscv/assembler/mod.hpp` and
  `src/backend/mir/aarch64/assembler/mod.hpp` are text-first external
  assembler staging APIs. They are useful as contrast, not as the compiler
  object API.
- `src/backend/mir/aarch64/assembler/types.hpp` is parser-shaped and explicitly
  says those records are not the future structured encoder/object contract.
- `src/backend/mir/aarch64/codegen/instruction.hpp` already models
  `ObjectInstructionRecord` with `RecordSurfaceKind::EncoderInput`; the
  existing `backend_aarch64_target_record_core_contract` test should remain as
  a guard that object emission is structured and assembler input is text-first.
- `src/backend/mir/riscv/assembler/elf_writer.cpp` has local little-endian
  append helpers, minimal `.text`/`.rela.text`/`.symtab` serialization, and a
  hard-coded RV64 JAL relocation slice. Its useful logic should be generalized,
  but its parser-shaped minimal-slice matching and RV64 constants should not
  become the shared compiler model.
- `src/backend/mir/riscv/linker/*` contains richer ELF/relocation concepts, but
  those files currently depend on missing `src/backend/elf` and
  `src/backend/linker_common` C++ surfaces and are not wired into
  `src/backend/CMakeLists.txt`; treat them as staging/reference, not as the
  direct API home.
- Rust reference files under `ref/claudes-c-compiler/src/backend/elf/` provide
  the best shape to port: `ElfConfig`, `ObjSection`, `ObjReloc`, `ObjSymbol`,
  string tables, section order, and relocatable writer separation.

Inspected commands/files:

- `rg --files src/backend/mir tests | rg '(riscv|aarch64|assembler|object|elf|prepared|codegen)'`
- `rg -n 'ELF|Elf|reloc|Reloc|object|Object|assembler|Assembler|section|symbol|Symbol' src/backend/mir tests -g '*.hpp' -g '*.cpp'`
- `sed -n` reads of `src/backend/mir/riscv/assembler/mod.hpp`,
  `src/backend/mir/riscv/assembler/parser.hpp`,
  `src/backend/mir/riscv/assembler/elf_writer.cpp`,
  `src/backend/mir/riscv/linker/mod.hpp`,
  `src/backend/mir/riscv/linker/README.md`,
  `src/backend/mir/aarch64/assembler/mod.hpp`,
  `src/backend/mir/aarch64/assembler/types.hpp`,
  `src/backend/mir/aarch64/codegen/instruction.hpp`,
  `src/backend/mir/aarch64/codegen/instruction.cpp`,
  `tests/backend/mir/backend_aarch64_target_record_core_contract_test.cpp`,
  `src/backend/CMakeLists.txt`, and `tests/backend/mir/CMakeLists.txt`
- clang lookup:
  `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/aarch64/codegen/instruction.cpp make_object_instruction build/compile_commands.json`
  found `make_object_instruction` at `src/backend/mir/aarch64/codegen/instruction.cpp:2047`.

## Suggested Next

Execute Step 2 first slice: add target-neutral object model records only,
without ELF serialization yet. The slice should define stable IDs and records
for object module, section, symbol, label, relocation, and target ELF config in
`src/backend/mir/object/model.hpp`/`.cpp`, wire them into `c4c_backend`, and add
`backend_object_model_records_test` proving `.text`, `.data`, `.bss`,
undefined symbols, local/global function symbols, section-local labels, and
numeric relocation records with addends can be represented without parser text.

## Watchouts

- Do not implement RV64 or AArch64 object emission in child 330.
- Do not add `--codegen obj` CLI behavior in child 330.
- Do not make the compiler object path depend on printed `.s` parsing.
- Keep target-specific relocation names and fixup semantics out of shared
  object model records.
- Preserve existing asm-route coverage.
- Shared records should store target-chosen numeric ELF relocation values only;
  RV64 names like `R_RISCV_PCREL_HI20` and AArch64 names like
  `R_AARCH64_CALL26` belong in later target children.
- Start with `model.*`; defer string tables, symbol-table ordering, relocation
  section emission, and ELF byte serialization to the Step 4 writer slice.
- Do not reuse `riscv::assembler::parser.hpp` records or
  `aarch64::assembler::types.hpp` as model inputs; those are external text
  parser records.

## Proof

Research-only packet; no build/test validation required and no `test_after.log`
written.

First focused implementation proof command:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^backend_object_model_records$' --output-on-failure) > test_after.log 2>&1
```
