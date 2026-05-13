# AArch64 Linker Module Legacy Surface

This artifact preserves the useful module-surface shape from the removed
`src/backend/mir/aarch64/linker/mod.cpp` translation unit. The old file was a
commented structural mirror of
`ref/claudes-c-compiler/src/backend/arm/linker/mod.rs`.

## Role

The removed file was the top-level module index for the old native AArch64
ELF64 linker. It did not contain live C++ linker logic; it described how the
reference Rust linker grouped target-specific AArch64 linker shards and which
entry points were re-exported when the GCC linker feature was disabled.

The module comment described a linker intended to consume ELF relocatable
objects and static archives, produce dynamically linked ELF64 executables for
AArch64, and also support shared-library output through `link_shared()`.

## Module Structure

The old module listed these linker shards:

- `elf`: ELF64 constants, type aliases, and parsing through shared
  `linker_common`
- `reloc`: AArch64 relocation application and instruction encoding helpers
- `types`: `GlobalSymbol`, `GlobalSymbolOps`, and architecture constants
- `input`: object, archive, shared-library, and linker-script input loading
- `plt_got`: PLT/GOT entry construction from relocation scanning
- `link`: orchestration for `link_builtin` and `link_shared`
- `emit_dynamic`: dynamic executable emission, including PLT/GOT and
  `.dynamic`
- `emit_shared`: shared-library emission
- `emit_static`: static executable emission

The old Rust-style visibility comments made `elf`, `reloc`, and `types`
public module surfaces, while `input`, `plt_got`, `link`, `emit_dynamic`,
`emit_shared`, and `emit_static` were private implementation shards.

## Exported Entry Points

The old module re-exported these linker entry points when `gcc_linker` was not
enabled:

- `link::link_builtin`
- `link::link_shared`

Those exports are historical module-boundary evidence. They do not prove that
the current C++ backend has a complete built-in AArch64 linker wired into the
compiler driver.

## Shared Infrastructure Assumptions

The module-level documentation assigned generic linker work to
`linker_common`, including:

- ELF parsing
- section merging
- symbol registration
- common-symbol allocation
- archive loading
- CRT object discovery and builtin library path setup through
  `resolve_builtin_link_setup`

The target-specific AArch64 module was expected to own:

- PLT/GOT construction
- relocation patching and instruction-field encoding
- address layout
- executable and shared-object emission

Future reconstruction should keep that boundary explicit instead of moving all
linker behavior into a target-local monolith.

## Cross-Surface Dependencies

This module index ties together the linker markdown artifacts already
extracted for Step 2.4:

- `linker/elf.md` records the old ELF facade and shared parser aliases.
- `linker/types.md` records architecture constants, `GlobalSymbol`, dynamic
  symbol state, PLT/GOT indexes, copy-relocation fields, and common/BSS
  behavior.
- `linker/input.md` records staged object and archive loading behavior.
- `linker/plt_got.md` records PLT/GOT classification, reserved GOT slots, and
  dynamic relocation list construction.
- `linker/reloc.md` records the first static text relocation patcher and
  supported AArch64 relocation encodings.
- `linker/link.md` records first-static orchestration, text merging, and entry
  point selection.
- `linker/emit_dynamic.md` records dynamic executable emission.
- `linker/emit_shared.md` records shared-object emission.
- `linker/emit_static.md` records minimal first-static executable emission.

The module list is an index over those surfaces, not a substitute for their
behavioral details.

## Hidden Assumptions

- The reference linker was the default path only when the `gcc_linker` feature
  was disabled.
- Dynamic executable, shared-library, and static executable outputs all shared
  the same conceptual module family, even though their implementation and proof
  requirements differ substantially.
- The old comments assumed `linker_common` owned enough shared ELF machinery to
  avoid duplicating parser, symbol, archive, and CRT setup behavior in the
  target module.
- Public/private markers came from Rust module visibility and should not be
  copied directly into C++ linkage or build-file decisions.

## Failure Risks For Rebuild

- Treating the module index as proof of live linker support would overstate the
  current AArch64 backend. It is reconstruction guidance only.
- Recreating every listed shard as compiled C++ before defining a tested driver
  boundary could revive stale structure without usable linker behavior.
- Flattening `linker_common` responsibilities into the AArch64 linker would
  duplicate parser and archive logic that is meant to remain shared across
  targets.
- Restoring only `link_builtin` without preserving `link_shared` and dynamic
  emission constraints would lose part of the old exported surface.
- Reusing Rust feature-gate comments mechanically in C++ could obscure the
  actual build and driver selection model used by this repository.

## Rebuild Guidance

Use this artifact as the Step 2.4 linker directory map. Any future live AArch64
linker work should start from a narrow tested integration surface, then pull
behavior from the extracted shard artifacts according to the shared
`linker_common` boundary and the desired output mode. It must remain downstream
of object/relocation records produced from structured machine instruction nodes
or lower encoding records; linker work is not a reason for codegen to route
through printed assembly text and parser recovery.
