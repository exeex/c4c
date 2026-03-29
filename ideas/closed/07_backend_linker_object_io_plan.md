# Backend Linker Object IO Plan

Status: Complete

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/04_backend_binary_utils_contract_plan.md`
- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`

Should precede:

- `ideas/open/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Implement the shared ELF object and archive reading support that the built-in linker will need before target-specific relocation and executable-layout work begins, starting with compile-time integration of the mirrored shared linker and ELF trees.

## Simplified Staging Note

This idea is now mainly the shared-linker compile and interface stage.

If the shared object/archive IO work and the first AArch64 linker wiring advance together, they can be treated as one larger binary-utils stream instead of two rigidly separate long-lived ideas.

For now, optimize this stage for unblocking and end-to-end bring-up rather than broad ELF coverage.
The immediate goal is to make one or two simple Linux ELF object/archive cases parse cleanly so later linker work has a dependable path to build on.

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/linker_common/README.md`
- `ref/claudes-c-compiler/src/backend/linker_common/types.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/parse_object.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/archive.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/symbols.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/merge.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/section_map.rs`
- `ref/claudes-c-compiler/src/backend/elf/archive.rs`
- `ref/claudes-c-compiler/src/backend/elf/io.rs`
- `ref/claudes-c-compiler/src/backend/elf/constants.rs`

## Why This Is Separate

- object and archive parsing are reusable linker foundations, not target-local finish work
- separating this layer keeps later linker plans focused on relocation semantics and executable layout
- it provides a stable place to validate symbol-table, section, and archive-member handling independently

## Ref Shape To Preserve

- keep shared ELF64 object and archive parsing in target-independent code, as ref does in `linker_common` and `elf`
- let target-specific linkers supply relocation semantics and special output details later, but do not make them each reparse object files independently
- keep the first shared boundary centered on:
  `Elf64Object`,
  `Elf64Section`,
  `Elf64Symbol`,
  `Elf64Rela`,
  `InputSection`,
  and `OutputSection`

## Scope

- make `src/backend/elf/` and `src/backend/linker_common/` include-clean enough to compile together as shared linker infrastructure
- read the bounded ELF relocatable objects needed by the first supported backend flow
- read static archives at the level needed for a simple symbol-resolution and member-extraction smoke path
- expose shared representations for sections, symbols, relocations, and archive contents
- expose enough section-name mapping and merge metadata that a later linker can reason about output-section construction without reparsing inputs
- add focused coverage for a small number of happy-path cases first; malformed-input coverage can stay narrow until the bring-up path is working

## Acceptance Stages

### Stage 1: Shared ELF/linker trees compile

- `elf/` and `linker_common/` compile as a shared dependency layer

### Stage 2: Shared object/archive interfaces are explicit

- later AArch64 linker code can include stable shared types and parsers

### Stage 3: Object/archive parsing starts working

- simple ELF objects and archives begin parsing through the shared layer end to end

## Suggested Execution Order

1. port low-level ELF binary IO and core ELF64 constants and types
2. port relocatable-object parsing into a stable shared `Elf64Object`-style representation
3. port archive parsing, including member enumeration and symbol-driven extraction paths needed for static linking
4. port the shared section and symbol bookkeeping types that later link steps consume
5. stop before relocation application or final executable writing

## Early Coverage Targets

- one plain relocatable object with a small section, symbol-table, and relocation inventory
- one single-member archive or similarly small archive case that proves the shared archive path is wired through
- optional next step: one multi-section object that exercises section-name normalization such as `.text.foo` to `.text`
- malformed ELF and malformed archive cases should stay narrow until the basic bring-up path is stable

## Explicit Non-Goals

- relocation application
- final executable writing
- broad malformed-input hardening before the first happy-path slice is reliable
- target-specific ABI policy beyond what object parsing requires
- external Linux stdlib linking

## Validation

- first validation gate: `elf/` and `linker_common/` compile and expose stable shared declarations
- second validation gate: later linker work can reuse parsed symbol, section, and relocation data without introducing target-specific object readers
- later validation: simple Linux ELF objects and archives can be parsed into stable shared linker data structures
- host-side development may run on macOS, but Linux ELF validation is allowed to run inside Docker

## Good First Patch

Parse one relocatable ELF object and one single-member archive into shared linker data structures, then lock the expected section, symbol, and relocation inventory in tests.

## Completion Notes

- The shared linker layer now parses one bounded relocatable ELF object and one bounded regular archive case into explicit shared `Elf64Object` and `ElfArchive`-style surfaces.
- `src/backend/elf/archive.cpp` now provides the first regular archive member enumeration surface, and `src/backend/linker_common/archive.cpp` now lifts those members into linker-facing archive/member inventory with defined-symbol lookup.
- Backend contract coverage now proves a single-member `libminimal.a` fixture end to end, including member-name normalization, parsed member-object preservation, relocation inventory retention, and symbol-driven lookup for `main`.

## Leftover Issues

- Thin-archive support and multi-member demand-driven extraction remain follow-on work and should stay outside this completed first archive IO slice.
- Full-suite regression results stayed monotonic at the same four unrelated failures:
  - `positive_sema_ok_fn_returns_variadic_fn_ptr_c`
  - `cpp_positive_sema_decltype_bf16_builtin_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_llvm_initializer_list_runtime_materialization`
