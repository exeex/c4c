# Built-in AArch64 Assembler Plan

Status: Complete

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/04_backend_binary_utils_contract_plan.md`
- `ideas/open/05_backend_builtin_assembler_boundary_plan.md`
- `ideas/open/02_backend_aarch64_port_plan.md`

Should precede:

- `ideas/open/07_backend_linker_object_io_plan.md`
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Implement the first built-in assembler slice for AArch64 so the compiler can emit working ELF objects without relying on an external assembler for the supported backend subset, but only after the mirrored assembler tree is compile-integrated and connected to the backend output boundary.

## Simplified Staging Note

This idea should now be read as the AArch64-specific continuation of the assembler compile-integration work, not as an immediate behavior-first project.

If the shared-boundary work and the AArch64-specific work move together in practice, they do not need to stay rigidly separate.

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/arm/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/arm/assembler/mod.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/parser.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/elf_writer.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/mod.rs`
- encoder slices that match the current backend subset:
  `encoder/data_processing.rs`,
  `encoder/compare_branch.rs`,
  `encoder/load_store.rs`,
  `encoder/system.rs`,
  `encoder/bitfield.rs`
- shared ELF helpers:
  `ref/claudes-c-compiler/src/backend/elf/`
  and `ref/claudes-c-compiler/src/backend/elf_writer_common.rs`

## Why AArch64 First

- it is already the first backend bring-up target
- it gives the earliest end-to-end proof that the built-in assembler boundary is viable
- later targets should reuse the same assembler architecture instead of discovering it independently

## Ref Shape To Preserve

- keep the ref three-stage pipeline visible:
  preprocess/parse assembly text into typed statements,
  encode instructions and collect relocations,
  write an ELF64 relocatable object
- keep target-local encoding in the AArch64 assembler rather than trying to genericize instruction encoding across targets
- reuse shared ELF/string-table/symbol-table machinery where ref already shares it

## Scope

- make the AArch64 assembler subtree under `src/backend/aarch64/assembler/` compile together with the shared ELF/assembler support it depends on
- implement the minimum instruction parsing or structured-instruction consumption required by the chosen assembler boundary
- encode the supported AArch64 instruction subset already exercised by backend tests
- start from the current backend-emitted subset rather than the full ref assembler:
  prologue and epilogue instructions,
  integer ALU basics,
  compare/branch,
  direct call and return,
  stack-relative loads/stores,
  `adrp` plus low12 materialization for globals,
  and a narrow data-directive set needed by current output
- support the relocation forms that naturally fall out of that subset before expanding farther
- write ELF relocatable objects for that supported subset
- validate emitted objects against external assembler output for representative cases

## Acceptance Stages

### Stage 1: AArch64 assembler subtree compiles

- parser, encoder, and ELF-writer pieces can be included together and built

### Stage 2: Backend and assembler boundary connect

- AArch64 backend output can be routed into the assembler entry point without ad hoc local hacks

### Stage 3: Object emission starts working

- the first supported subset begins producing comparable ELF relocatable objects

## Suggested Execution Order

1. port only enough parser surface to cover the AArch64 codegen's current emitted assembly subset
2. port the smallest encoder slices matching that subset:
   data processing,
   compare/branch,
   load/store,
   system/return,
   and address-materialization relocations
3. port the AArch64-specific object-writing layer around shared ELF helpers
4. compare the resulting `.o` files against external assembly on section layout, symbols, relocations, and disassembly
5. widen parser and encoder coverage only when new backend slices require it

## Early Relocation Focus

- direct call relocations
- branch relocations
- `adr` / `adrp` style PC-relative materialization
- low-12 add/load/store relocations for globals and literal-addressing patterns
- defer broader TLS, GOT, IFUNC, large NEON, and dynamic-linking relocation coverage until the linker path needs them

## Explicit Non-Goals

- x86-64 or rv64 assembler support
- linker implementation
- assembler support beyond the already-supported AArch64 backend subset

## Validation

- first validation gate: the AArch64 assembler subtree compiles and is wired to the chosen assembler entry boundary
- second validation gate: emitted AArch64 object files disassemble correctly
- later validation: object metadata, sections, and relocations are comparable to externally assembled output for covered cases

## Good First Patch

Make parser, encoder, and ELF-writer pieces compile together, then route one minimal backend-emitted AArch64 function through that path.

## Completion Notes

- The AArch64 built-in assembler now emits one real ELF relocatable object slice for backend-emitted `return_add`, keeping the flow text-first through parse, encode, and ELF-write stages.
- Backend contract coverage now checks the bounded object-emission slice end to end: `.text` size, global `main` symbol presence, relocation-free metadata, and matching disassembly against the external assembler baseline for the same emitted assembly.
- The active backend adapter and production `assemble_module(..., output_path)` handoff both continue to report object emission through the named assembler request/result seam.

## Leftover Issues

- Relocation-bearing slices remain follow-on work for later ideas and should not be folded back into this completed first-slice runbook.
- Full-suite regression results remain monotonic at the same four unrelated failures:
  - `positive_sema_ok_fn_returns_variadic_fn_ptr_c`
  - `cpp_positive_sema_decltype_bf16_builtin_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_llvm_initializer_list_runtime_materialization`
