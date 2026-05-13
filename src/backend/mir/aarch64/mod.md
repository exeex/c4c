# AArch64 Top-Level Module Legacy Surface

This artifact preserves the useful module-surface shape from the removed
`src/backend/mir/aarch64/mod.cpp` translation unit. The old file was a
commented structural mirror of
`ref/claudes-c-compiler/src/backend/arm/mod.rs`.

## Role

The removed file was the top-level index for the old AArch64 backend family.
It did not contain live C++ lowering, assembly, or linking logic. Instead, it
recorded how the reference Rust backend grouped the target-specific codegen,
assembler, and linker submodules and which codegen type was re-exported for
callers.

This surface is a directory map for reconstruction. It shows that the old
backend treated instruction lowering, built-in assembly, and built-in linking
as one target family, with feature gates deciding whether the local assembler
or linker were expected to participate.

## Module Structure

The old module listed these AArch64 shards:

- `codegen`: target lowering and assembly-text emission
- `assembler`: built-in assembler support, conditionally unused when a GCC
  assembler feature handles assembly
- `linker`: built-in linker support, conditionally unused when a GCC linker
  feature handles linking

The module comments made `codegen`, `assembler`, and `linker` crate-visible in
the reference Rust layout. Those visibility markers are historical module
boundary evidence only; they do not establish current C++ linkage or build-file
policy.

## Exported Surface

The old top-level module re-exported:

- `codegen::emit::ArmCodegen`

That export identified the reference backend's primary codegen entry type. In
the current markdown-first reconstruction, it should be read as evidence that
the live integration point belongs above the detailed codegen shard artifacts,
not as proof that an AArch64 C++ `ArmCodegen` implementation is still compiled
or driver-wired.

## Cross-Surface Dependencies

This top-level module ties together the Step 2 AArch64 extraction artifacts:

- `codegen/mod.md` records the old codegen shard index and its lowering family.
- `assembler/mod.md` records the old assembler orchestration surface, including
  literal-pool expansion, numeric-label resolution, and object writer entry
  points.
- `assembler/encoder/mod.md` records the old encoder module index and
  instruction-family split.
- `linker/mod.md` records the old linker shard index, exported linker entry
  points, and shared `linker_common` boundary.

The top-level index does not replace those artifacts. It only records how the
old reference backend grouped them under one target namespace.

## Feature-Gate Assumptions

The comments carried two reference-feature assumptions:

- `gcc_assembler` allowed the built-in assembler module to be dead code when
  an external assembler handled that stage.
- `gcc_linker` allowed the built-in linker module to be dead code when an
  external linker handled that stage.

Those assumptions describe the reference Rust backend's configuration model.
They should not be copied mechanically into C++ preprocessor or CMake policy
without first defining the current repository's target-selection contract.

## Hidden Assumptions

- The file was descriptive only; every module declaration and export was
  commented out.
- A single top-level backend namespace was expected to own codegen, assembler,
  and linker integration decisions.
- The old export named `ArmCodegen` even though this repository path is
  `aarch64`; preserve that spelling as historical source evidence, not as a
  new naming requirement.
- The presence of assembler and linker modules does not prove that current live
  AArch64 backend behavior includes built-in object emission or linking.
- Rust crate visibility and feature attributes are planning signals, not live
  C++ access-control or build-system facts.

## Failure Risks For Rebuild

- Treating this top-level index as complete backend behavior would overstate
  the current tree; the actual lowering, assembly, and linking contracts live
  in the extracted shard artifacts and future implementation.
- Rebuilding all three target families at once would blur integration risk
  between codegen, object emission, and linking. A future live route should
  choose one tested entry boundary first.
- Mechanically restoring feature gates could hide whether the compiler driver
  is using local AArch64 stages or external toolchain stages.
- Renaming the exported `ArmCodegen` surface without recording compatibility
  intent could lose evidence about the reference backend's public API shape.
- Leaving assembler/linker assumptions implicit could make backend tests appear
  to prove more than they actually cover.

## Rebuild Guidance

Use this artifact as the Step 2.5 top-level AArch64 directory map. Future live
AArch64 work should define the driver-facing target entry point first, then
pull codegen, assembler, and linker behavior from their detailed markdown
artifacts according to an explicit toolchain-selection policy. The codegen
side of that entry must route prepared target MIR facts into structured machine
instruction nodes; `.s` printing, external assembler parsing, encoding,
object writing, and linking are downstream consumers or external-input paths,
not internal semantic recovery routes.
