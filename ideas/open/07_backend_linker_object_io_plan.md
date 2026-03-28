# Backend Linker Object IO Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/04_backend_binary_utils_contract_plan.md`
- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`

Should precede:

- `ideas/open/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Implement the shared ELF object and archive reading support that the built-in linker will need before target-specific relocation and executable-layout work begins.

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

- read ELF relocatable objects needed by the supported backend flow
- read static archives at the level needed for symbol resolution and member extraction
- expose shared representations for sections, symbols, relocations, and archive contents
- expose enough section-name mapping and merge metadata that a later linker can reason about output-section construction without reparsing inputs
- add focused coverage for malformed-input rejection and representative happy-path cases

## Suggested Execution Order

1. port low-level ELF binary IO and core ELF64 constants and types
2. port relocatable-object parsing into a stable shared `Elf64Object`-style representation
3. port archive parsing, including member enumeration and symbol-driven extraction paths needed for static linking
4. port the shared section and symbol bookkeeping types that later link steps consume
5. stop before relocation application or final executable writing

## Early Coverage Targets

- one plain relocatable object with `.text`, `.rodata`, `.data`, `.bss`, symbol table, and relocations
- one multi-section object that exercises section-name normalization such as `.text.foo` to `.text`
- one single-member archive and one small archive that requires iterative member discovery by unresolved symbols
- malformed ELF and malformed archive cases that should fail narrowly

## Explicit Non-Goals

- relocation application
- final executable writing
- target-specific ABI policy beyond what object parsing requires

## Validation

- representative ELF objects and archives can be parsed into stable shared linker data structures
- malformed inputs fail narrowly and diagnostically
- later linker work can reuse parsed symbol, section, and relocation data without introducing target-specific object readers
- the later built-in linker plan can depend on this layer instead of reparsing binary formats ad hoc

## Good First Patch

Parse one relocatable ELF object and one single-member archive into shared linker data structures, then lock the expected section, symbol, and relocation inventory in tests.
