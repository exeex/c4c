# Built-in x86 Linker Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/04_backend_binary_utils_contract_plan.md`
- `ideas/closed/23_backend_builtin_assembler_x86_plan.md`

## Goal

Implement the first built-in linker slice for x86 so supported programs can be linked into working executables without relying on the external linker, but only after the mirrored shared linker layers and x86 linker subtree are compile-integrated.

## Why This Comes Last

- linker correctness depends on both a stable assembler contract and shared object/archive reading support
- relocation application, PLT/GOT setup, symbol resolution, and executable layout create a larger correctness surface than assembler object emission alone
- the first linker should follow a bounded backend-plus-assembler path rather than expand codegen scope

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/x86/linker/README.md`
- `ref/claudes-c-compiler/src/backend/x86/linker/mod.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/link.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/input.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/elf.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/types.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/emit_exec.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/emit_shared.rs`
- `ref/claudes-c-compiler/src/backend/x86/linker/plt_got.rs`

## Scope

- make the mirrored x86 linker subtree under `src/backend/x86/linker/` compile together with `src/backend/elf/` and `src/backend/linker_common/`
- resolve symbols across a bounded set of supported objects and archives
- apply the x86 relocation subset needed by the first simple backend/linker tests
- lay out a working ELF executable for the supported subset
- emit the first working static executable path for simple bring-up cases

## Explicit Non-Goals

- shared-library or dynamic-linker features as a first-stage requirement
- full libc/static-system-link coverage as the first success bar
- rv64 or AArch64 linker support

## Suggested Execution Order

1. port `link.rs`-style orchestration for static linking first
2. reuse shared object and archive parsing plus shared symbol-registration logic
3. implement the narrow x86 relocation subset needed by current backend tests
4. port a minimal static executable emitter
5. validate executable layout and simple runtime behavior against the external linker path
6. defer dynamic linking, shared libraries, PLT/GOT polish, TLS, and wider relocation coverage until the static slice is stable

## Validation

- first validation gate: the x86 linker subtree compiles against the shared linker layers
- second validation gate: the first slice links a tiny bounded x86 program into one working ELF image
- later validation: simple linked executables run equivalently to externally linked outputs for covered cases

## Good First Patch

Link a tiny multi-object x86 program with one relocation-bearing call or global reference, then compare the resulting executable layout and runtime behavior against the external linker path.
