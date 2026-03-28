# Backend Binary Utils Contract Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Should remain later than:

- `ideas/open/02_backend_aarch64_port_plan.md`
- `ideas/open/03_backend_regalloc_peephole_port_plan.md`

Should precede:

- `ideas/open/05_backend_builtin_assembler_boundary_plan.md`
- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`
- `ideas/open/07_backend_linker_object_io_plan.md`
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Document and stage the exact assembler/linker contract currently provided by the external toolchain so the later built-in replacements have a concrete compatibility target, starting with compile-time/shared-interface readiness rather than full binary behavior.

## Simplified Staging Note

This idea is now mainly a contract-and-acceptance bucket for the binary-utils track.

If the assembler and linker compile-integration work ends up moving as one stream, this idea can be merged conceptually into the later binary-utils ideas instead of staying a long-lived standalone plan.

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/common.rs`
- `ref/claudes-c-compiler/src/backend/arm/asm_stub.sh`
- `ref/claudes-c-compiler/src/backend/arm/ld_stub.sh`
- `ref/claudes-c-compiler/src/backend/arm/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/arm/linker/README.md`
- shared ELF and archive helpers under:
  `ref/claudes-c-compiler/src/backend/elf/`
  and `ref/claudes-c-compiler/src/backend/linker_common/`

## Why This Comes First

- built-in binary utils should replace a known contract, not an implied one
- golden contract coverage reduces ambiguity before assembler or linker implementation begins
- later plans can compare behavior against stable fixtures instead of reverse-engineering expectations repeatedly

## Ref Details To Capture

- ref codegen produces raw GNU-style assembly text first, then assembles and links as separate stages
- ref still supports external assembler/linker fallback via thin target-local stubs; the replacement contract should stay explicit in the C++ port too
- the compatibility target is not just "an executable runs":
  it includes section naming,
  symbol typing and visibility,
  numeric-label behavior,
  relocation shapes,
  archive member loading,
  and executable entry/layout expectations

## Scope

- identify which shared declarations, data structures, and interface boundaries need to exist before the built-in assembler/linker trees can even be compiled into the build
- identify the current backend outputs handed to the external assembler and linker
- record the exact assembly constructs the current backend relies on, including:
  labels,
  directives,
  symbol references,
  relocation-triggering instruction forms,
  and data directives used for globals or literal data
- record required calling conventions for file names, symbol visibility, sections, relocations, and executable entry expectations
- document the object-file properties later plans must preserve:
  ELF class,
  relocatable-object expectations,
  section inventory,
  symbol-table essentials,
  and relocation-table essentials
- document the executable-side contract later built-in linker work must preserve:
  entry symbol expectations,
  startup object ordering if relevant,
  static-vs-dynamic assumptions,
  and minimal supported library/archive behavior
- add focused tests or fixtures that lock the current external toolchain behavior where practical
- make failure attribution between backend emission, assembler behavior, and linker behavior explicit

## Acceptance Stages

### Stage 1: Contract headers and interfaces exist

- the shared ELF/archive/object/linker-facing interfaces needed by later binary-utils code are declared and include-reachable
- assembler/linker trees can refer to a bounded contract surface instead of ad hoc local declarations

### Stage 2: Toolchain boundary is documented

- the repo records the current `.s -> .o -> executable` contract clearly enough for later built-in replacements

### Stage 3: Golden behavior is locked

- representative external-toolchain cases are captured in tests or fixtures

## Suggested Execution Order

1. inventory the exact `.s -> .o -> executable` path currently used by the AArch64 backend
2. capture one tiny single-object case and one small multi-object relocation-bearing case
3. record which GAS constructs and ELF details are actually exercised by those cases
4. lock those cases in repo tests or golden inspection fixtures before changing the toolchain boundary
5. use this contract note as the citation target for the later assembler and linker ideas

## Explicit Non-Goals

- implementing a built-in assembler
- implementing a built-in linker
- broad backend refactors unrelated to the external toolchain boundary

## Validation

- first validation gate: later assembler/linker code can include a bounded shared contract surface without inventing local duplicates
- second validation gate: the repo contains a concrete description of the current assembler/linker contract
- later validation: targeted tests or golden fixtures cover representative object and executable cases
- follow-on binary-utils work can cite this idea as the compatibility baseline

## Good First Patch

Capture one minimal end-to-end object and executable flow, including emitted assembly, produced object sections, and linked symbol expectations, so later built-in tools can compare against it directly.
