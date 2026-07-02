# RV64 Object Emission Cleanup Umbrella

Status: Open
Type: Analysis umbrella idea
Order: after `ideas/open/518_bir_core_model_cleanup_umbrella.md`, before other implementation ideas
Owning Layer: RV64 MIR object emission
Primary File: `src/backend/mir/riscv/codegen/object_emission.cpp`
Reference Shape: `src/backend/mir/aarch64/codegen/`

## Goal

Analyze `src/backend/mir/riscv/codegen/object_emission.cpp` and produce a
follow-up cleanup plan that splits the 12k-line RV64 object emission path into
reviewable ownership areas, using the existing AArch64 codegen organization as
the main structural reference.

## Why This Exists

`object_emission.cpp` is currently the largest source file in `src/` at roughly
12.7k lines. It combines ELF/object constants, raw instruction encoding,
prepared-module admission diagnostics, stack-frame checks, variadic helpers,
prepared move emission, local/global memory emission, scalar/control lowering,
call handling, symbol/fixup handling, object data emission, and final object
module assembly.

The AArch64 codegen tree already demonstrates a healthier split across
`calls`, `memory`, `globals`, `returns`, `prologue`, `variadic`,
`object_emission`, `module_compile`, and related focused files. RV64 should
not blindly copy AArch64, but the cleanup plan should use that split as the
first comparison point.

## In Scope

- Inventory major regions and helper families in RV64 `object_emission.cpp`.
- Compare those regions with the AArch64 codegen file layout.
- Identify which RV64 regions already have focused files that should receive
  code, such as:
  - `prepared_call_emit.cpp`;
  - `prepared_frame_emit.cpp`;
  - `prepared_function_emit.cpp`;
  - `prepared_global_memory_emit.cpp`;
  - `prepared_local_memory_emit.cpp`;
  - `prepared_scalar_emit.cpp`;
  - `prepared_edge_publication_emit.cpp`;
  - `rv64_line_assembler.cpp`;
  - `returns.cpp`, `prologue.cpp`, `variadic.cpp`, `calls.cpp`, `memory.cpp`,
    `globals.cpp`, and related RV64 codegen files.
- Identify new files only where existing files do not express the right
  ownership.
- Produce a staged follow-up cleanup plan that keeps behavior unchanged.
- Define validation scope for each future split slice.
- Preserve the ordering:
  `422 -> BIR cleanup analysis -> RV64 emission cleanup analysis -> other idea`.

## Out Of Scope

- Do not move RV64 implementation code in this umbrella.
- Do not change instruction encodings, ELF/object emission, relocation
  behavior, stack-frame layout, prepared handoff admission, or diagnostics.
- Do not repair gcc_torture failures as part of this analysis.
- Do not add target-side inference for missing prepared or BIR facts.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Do not copy AArch64 structure mechanically when RV64 prepared-object
  ownership requires a different split.

## Required Analysis Artifact

The runbook should produce a durable plan artifact under
`docs/rv64_object_emission_cleanup/` or
`build/agent_state/519_rv64_object_emission_cleanup/` that contains:

- current line-count and region map for `object_emission.cpp`;
- AArch64 comparison table;
- proposed destination map for each RV64 region;
- proposed new-file list, if any;
- staged follow-up idea list with owned files and validation commands;
- risk notes for symbol/fixup handling, prepared admission, and runtime
  behavior.

## Follow-Up Plan Shape

The cleanup plan should prefer behavior-preserving slices such as:

1. Extract pure encoding and byte/fixup append helpers when they can be tested
   without changing prepared lowering behavior.
2. Move prepared admission and diagnostic helpers behind focused prepared
   object emission files.
3. Move local/global memory, scalar, call, variadic, prologue, return, and
   object-data regions toward existing RV64 focused files where ownership is
   already present.
4. Keep final object-module assembly and cross-cutting symbol/fixup ownership
   narrow and explicit.
5. Split validation so each movement slice has matching build and focused
   backend proof before any broader gcc_torture evidence is cited.

## Acceptance Criteria

- The umbrella produces an explicit, reviewable RV64 object emission cleanup
  plan.
- The plan references the AArch64 split and records where RV64 should differ.
- The plan names concrete follow-up ideas and their owned files.
- The plan identifies high-risk regions that should move late or stay central.
- No implementation files are changed except for lifecycle or documentation
  artifacts required by the analysis.
- The active plan remains unchanged unless this idea is explicitly activated
  later.

## Reviewer Reject Signals

Reject this umbrella or any claimed progress if the slice:

- moves RV64 implementation code while claiming to be analysis-only;
- changes RV64 object bytes, relocations, stack layout, prepared diagnostics,
  runtime behavior, or gcc_torture expectations;
- presents helper renames, file moves, or expectation churn as backend
  capability progress;
- creates a giant all-at-once split without staged ownership and validation;
- copies AArch64 layout blindly where RV64 prepared-object flow has different
  ownership;
- hides the same monolithic coupling behind new filenames without clear
  caller boundaries;
- adds testcase-shaped lowering shortcuts, target-side inference from missing
  prepared facts, or named gcc_torture case handling.
