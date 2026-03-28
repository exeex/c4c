# Built-in AArch64 Linker Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/04_backend_binary_utils_contract_plan.md`
- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`
- `ideas/open/07_backend_linker_object_io_plan.md`

## Goal

Implement the first built-in linker slice for AArch64 so supported programs can be linked into working executables without relying on the external linker, but only after the mirrored shared linker layers and AArch64 linker subtree are compile-integrated.

## Simplified Staging Note

This idea is now the AArch64-specific continuation of the shared linker bring-up.

If the shared-linker work and AArch64-specific linker wiring move together, they can be executed as one larger later-stage idea rather than forced into separate active runs.

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/arm/linker/README.md`
- `ref/claudes-c-compiler/src/backend/arm/linker/mod.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/link.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/input.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/elf.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/types.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/reloc.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/emit_static.rs`
- later-but-not-first-slice references:
  `ref/claudes-c-compiler/src/backend/arm/linker/emit_dynamic.rs`,
  `ref/claudes-c-compiler/src/backend/arm/linker/emit_shared.rs`,
  `ref/claudes-c-compiler/src/backend/arm/linker/plt_got.rs`
- shared linker infrastructure under:
  `ref/claudes-c-compiler/src/backend/linker_common/`

## Why This Comes Last

- linker correctness depends on both a stable assembler contract and shared object/archive reading support
- relocation application, symbol resolution, and executable layout create a larger correctness surface than assembler object emission alone
- doing the first linker after the AArch64 assembler keeps scope aligned with the already-prioritized target path

## Ref Shape To Preserve

- keep `link.rs`-style orchestration separate from:
  input loading,
  shared symbol and section merging,
  AArch64 relocation application,
  and executable emission
- use shared `linker_common` machinery for symbol registration, archive loading, section merging, and undefined-symbol checks
- keep AArch64-specific logic focused on:
  relocation semantics,
  page and alignment layout choices,
  and any AArch64-only PLT/GOT or TLS follow-ons
- start from the static executable path in `emit_static.rs` before taking on dynamic-linking or shared-library emission

## Scope

- make the mirrored AArch64 linker subtree under `src/backend/aarch64/linker/` compile together with `src/backend/elf/` and `src/backend/linker_common/`
- resolve symbols across supported objects and archives
- apply the AArch64 relocation subset needed by current backend coverage
- lay out a working ELF executable for the supported subset
- emit the first working static executable path for the supported subset
- compare linked outputs against the current external linker behavior for representative cases

## Acceptance Stages

### Stage 1: AArch64 linker subtree compiles

- `aarch64/linker/` compiles against the shared ELF/linker layers

### Stage 2: Shared and target-local linker layers connect

- AArch64 linker entry points can include and call the shared object/archive/symbol infrastructure

### Stage 3: Static link path starts working

- the first static executable path begins linking bounded AArch64 cases

## Suggested Execution Order

1. port `link.rs`-style orchestration for static linking only
2. reuse the shared object and archive parser plus shared symbol-registration logic
3. implement the narrow AArch64 relocation subset needed by current backend tests:
   calls,
   branches,
   PC-relative address materialization,
   and low12 add/load/store relocations
4. port a minimal static executable emitter modeled on `emit_static.rs`
5. validate runtime behavior and executable layout against the external linker path
6. defer dynamic linking, shared libraries, PLT/GOT, TLS, IFUNC, and `.eh_frame_hdr` polish until the static slice is stable

## Early Output Contract

- one executable entry path with the expected AArch64 ELF64 Linux headers
- merged `.text`, `.rodata`, `.data`, and `.bss` output sections for the supported subset
- symbol resolution across multiple objects and bounded archive use
- correct relocation application for the initial AArch64 backend-generated object set

## Explicit Non-Goals

- x86-64 or rv64 linker support
- shared-library or dynamic-linker features
- archive handling beyond what static linking of the supported subset requires

## Validation

- first validation gate: the AArch64 linker subtree compiles against the shared linker layers
- second validation gate: the first slice is explicitly static-link-first and tied to concrete `arm/linker/*.rs` reference modules
- later validation: linked executables run equivalently to externally linked outputs for covered cases

## Good First Patch

Link a tiny multi-object AArch64 program with one relocation-bearing call or global reference, then compare the resulting executable layout and runtime behavior against the external linker path.
