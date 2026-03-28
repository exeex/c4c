# Built-in AArch64 Assembler Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/04_backend_binary_utils_contract_plan.md`
- `ideas/open/05_backend_builtin_assembler_boundary_plan.md`
- `ideas/open/02_backend_aarch64_port_plan.md`

Should precede:

- `ideas/open/07_backend_linker_object_io_plan.md`
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Implement the first built-in assembler slice for AArch64 so the compiler can emit working ELF objects without relying on an external assembler for the supported backend subset.

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

- emitted AArch64 object files disassemble correctly
- object metadata, sections, and relocations are comparable to externally assembled output for covered cases
- the supported assembler subset is described in terms of concrete ref encoder/parser modules rather than a vague AArch64 support label
- the supported AArch64 backend slice can stop depending on an external assembler

## Good First Patch

Assemble one minimal AArch64 return/arithmetic function into an ELF relocatable object and compare its disassembly plus relocation table against the external assembler result.
