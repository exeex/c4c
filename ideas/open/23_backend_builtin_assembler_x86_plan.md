# Built-in x86 Assembler Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/15_backend_x86_port_plan.md`
- `ideas/closed/04_backend_binary_utils_contract_plan.md`

Should precede:

- `ideas/open/24_backend_builtin_linker_x86_plan.md`

## Goal

Implement the first built-in assembler slice for x86 so the compiler can emit working ELF objects without relying on an external assembler for the supported x86 backend subset.

## Why This Is Separate

- codegen bring-up and assembler object emission are distinct seams
- x86 instruction encoding coverage is broad enough that the first built-in assembler slice must stay tightly bounded
- the built-in assembler should follow one explicit backend-owned x86 assembly subset instead of driving codegen scope

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/x86/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/x86/assembler/mod.rs`
- `ref/claudes-c-compiler/src/backend/x86/assembler/parser.rs`
- `ref/claudes-c-compiler/src/backend/x86/assembler/elf_writer.rs`
- `ref/claudes-c-compiler/src/backend/x86/assembler/encoder/`

## Scope

- make the x86 assembler subtree under `src/backend/x86/assembler/` compile together with the shared ELF support it depends on
- encode the minimum x86 instruction subset already exercised by current backend tests
- write ELF relocatable objects for that supported subset
- validate emitted objects against external assembler output for representative cases

## Explicit Non-Goals

- linker implementation
- full x86 instruction-set coverage
- assembler support beyond the already-supported x86 backend subset

## Suggested Execution Order

1. port only enough parser surface to cover the current x86 codegen output subset
2. port the smallest encoder slices matching that subset
3. port the object-writing layer around shared ELF helpers
4. compare resulting `.o` files against external assembly on sections, symbols, relocations, and disassembly
5. widen parser and encoder coverage only when new x86 backend slices require it

## Validation

- the x86 assembler subtree compiles and is wired to the chosen assembler entry boundary
- emitted x86 object files disassemble correctly for the bounded subset
- object metadata and relocations are comparable to externally assembled output for covered cases

## Good First Patch

Make parser, encoder, and ELF-writer pieces compile together, then route one minimal x86 backend-emitted function through that path.
