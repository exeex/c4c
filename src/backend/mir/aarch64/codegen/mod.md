# AArch64 Codegen Module Legacy Surface

This artifact preserves the useful module-surface shape from the removed
`mod.cpp` translation surface. The old file was not active C++; it was a tiny
commented mirror of the reference Rust module declaration at
`ref/claudes-c-compiler/src/backend/arm/codegen/mod.rs`.

## Role

The surface described the old AArch64 codegen module boundary. It did not
contain executable lowering logic; it only listed the translation shards that
the reference backend grouped under `backend::arm::codegen`.

The file existed as an ownership map for the markdown-first reconstruction
lane. Each commented module line pointed at a separate lowering or cleanup
surface whose behavior must be archived or rebuilt independently.

## Module Surface

The removed module listed these codegen shards:

- `emit`
- `peephole`
- `asm_emitter`
- `f128`
- `inline_asm`
- `intrinsics`
- `prologue`
- `memory`
- `alu`
- `comparison`
- `calls`
- `globals`
- `cast_ops`
- `variadic`
- `returns`
- `atomics`
- `i128_ops`
- `float_ops`

Only `emit` and `peephole` were marked as public-to-the-crate in the old
Rust-style comments. The rest were private module entries. That visibility
distinction is a historical module-boundary note, not live C++ visibility.

## Dependencies And Ordering

The module file depended only on C++ namespace structure:

- `#include <string>` was present but did not support any live code.
- The namespace was `c4c::backend::aarch64::codegen`.
- All module declarations were comments, so the file did not force any
  translation-unit dependency in the current build.

The listed shards imply the old backend layering:

1. low-level assembly emission and string cleanup (`emit`, `asm_emitter`,
   `peephole`)
2. ABI and frame concerns (`prologue`, `calls`, `returns`, `variadic`)
3. scalar and memory instruction families (`alu`, `comparison`, `memory`,
   `globals`, `cast_ops`, `float_ops`)
4. special feature surfaces (`f128`, `i128_ops`, `atomics`, `intrinsics`,
   `inline_asm`)

The module list alone did not define pass order or call direction. Those
relationships belong in the extracted shard artifacts and any later rebuilt
implementation.

## Hidden Assumptions

- The file was a descriptive index, not a compilable implementation surface.
- The old AArch64 reconstruction should preserve shard boundaries before
  deciding which pieces become live C++ again.
- A commented module entry is evidence that the reference backend had a related
  Rust module, not evidence that the current C++ tree has live behavior for it.
- Public/private markers from the Rust comments only describe reference-module
  exposure and should not be copied mechanically into C++ linkage decisions.
- Deleted or archived shard `.cpp` files may still matter as design artifacts
  even when no build metadata references them.

## Failure Risks For Rebuild

- Treating this module list as a complete dependency graph would miss
  cross-shard contracts recorded inside the individual artifacts.
- Recreating all listed modules as live translation units before defining a new
  AArch64 lowering entry point would revive old text-first structure without a
  tested integration boundary.
- Dropping private shards from planning just because the old module file did
  not expose them publicly would lose ABI, memory, scalar, and special-feature
  behavior.
- Copying the stale `#include <string>` into rebuilt code would not establish
  the actual dependencies needed by the new implementation.

## Rebuild Guidance

Use this artifact as a directory map only. Rebuild decisions should come from
the detailed markdown artifacts for each shard plus the later top-level
AArch64 module-entry surface, not from this commented module file by itself.
The accepted rebuild route is structured target MIR records followed by
machine instruction nodes. Assembly-text emission is a printer consumer of
those nodes, and encoder/object work must consume nodes or lower structured
encoding records instead of parsing codegen-emitted text.
