# Native Object Model And Emission API Runbook

Status: Active
Source Idea: ideas/open/330_native_object_model_and_emission_api.md
Activated from: ideas/open/329_native_object_emission_umbrella.md

## Purpose

Introduce the shared native object-emission model and compiler-facing API that
RV64 and AArch64 can both use to produce relocatable ELF objects directly,
without making the compiler's object path depend on printed assembly text.

## Goal

Build the target-neutral object model and minimal ELF serialization surface
needed by later RV64 and AArch64 object-emission children.

## Core Rule

Keep target instruction encoding and target relocation semantics out of the
shared layer. Targets own typed fixups and lower them into target-neutral ELF
relocation records only after choosing the target ELF relocation number.

## Read First

- `ideas/open/330_native_object_model_and_emission_api.md`
- `ideas/open/329_native_object_emission_umbrella.md`
- existing backend emission and object-related helpers discovered under
  `src/backend/mir/`
- existing tests for backend prepared output, asm-route behavior, and any
  object/ELF helper coverage discovered during implementation

## Current Scope

- Add shared object module, section, symbol, label, relocation, and target ELF
  configuration records.
- Add compiler-facing helpers for byte append, alignment, label binding,
  symbol publication, and relocation attachment.
- Add minimal relocatable ELF serialization support for the first target
  object children.
- Add focused tests for object-model construction and ELF writer invariants.
- Preserve existing asm-route behavior and tests.

## Non-Goals

- Do not encode RV64 or AArch64 instructions in this child.
- Do not implement `--codegen obj` CLI behavior.
- Do not change c-testsuite defaults or backend route selection.
- Do not parse `.s` text or build a GNU-compatible assembler.
- Do not replace existing asm writers or weaken asm-route coverage.
- Do not implement RV64, AArch64, CLI, or c-testsuite object routing inside
  this shared-model child.

## Working Model

The shared layer should represent target-neutral object structure:

```text
target machine emission -> target typed fixups -> shared object records -> ELF
```

Targets remain responsible for instruction bytes, target-specific relocation
choice, and fixup pairing rules. The shared object model owns stable section
and symbol identities, label binding, append-only section contents, alignment,
relocation records with addends, and ELF table/section serialization.

## Execution Rules

- Prefer new shared object APIs over growing private target-local ELF writers.
- Keep shared type names target-neutral; do not leak RV64 concepts such as
  `pcrel_hi`, `PcrelLo12I`, or RVC policy into common records.
- Treat labels, symbols, and relocations as structured records, not strings or
  late byte-patch side channels.
- Add tests with each meaningful API or writer slice.
- Run at least a fresh build plus focused object-model/ELF tests for code
  slices; preserve asm-route tests when touched or when blast radius warrants.

## Step 1: Inspect Current Object And Backend Surfaces

Goal: identify the correct shared home for native object records and avoid
duplicating target-specific assembler internals.

Primary targets:

- `src/backend/mir/`
- `src/backend/mir/riscv/assembler/`
- existing backend and object-related tests

Actions:

- Inspect existing object-like records, ELF writer helpers, and assembler
  internals before adding new APIs.
- Identify the smallest shared namespace and file layout that both RV64 and
  AArch64 children can depend on.
- Record any existing tests that should be extended instead of duplicated.
- Keep findings in `todo.md`; do not rewrite the source idea unless its
  durable intent is wrong.

Completion check:

- `todo.md` names the chosen shared API location, relevant existing helpers or
  gaps, and the focused proof command for the first implementation slice.

## Step 2: Add Shared Object Model Records

Goal: introduce the target-neutral data model for compiler-produced object
modules.

Actions:

- Add records for object modules, sections, symbols, labels or stable section
  offsets, relocations with addends, and target ELF configuration.
- Provide stable identifiers for sections and symbols instead of raw names as
  the only linking mechanism.
- Model `.text`, `.data`, `.bss`, undefined symbols, local/global function
  symbols, section-local labels, and relocations.
- Keep target-specific relocation enums and fixup pairing outside shared
  record definitions.
- Add focused unit tests for construction invariants.

Completion check:

- The object model can express the acceptance-scope sections, symbols, labels,
  and relocations without depending on printed assembly text or parser records,
  and the focused model tests pass.

## Step 3: Add Compiler-Facing Construction Helpers

Goal: make the model usable from target machine emitters without exposing raw
ELF table details at call sites.

Actions:

- Add helpers for section creation or lookup, byte append, alignment, label
  binding, symbol publication, and relocation attachment.
- Keep helper behavior deterministic and suitable for later RV64/AArch64
  emission.
- Add tests for helper behavior, including label binding and relocation
  attachment with addends.

Completion check:

- Tests prove target-like clients can build a minimal object module through
  structured helpers rather than manual table mutation or string patching.

## Step 4: Add Minimal Relocatable ELF Serialization

Goal: serialize the shared model into structurally valid relocatable ELF for
minimal target configurations.

Actions:

- Add string table, symbol table, section header, and relocation-section
  serialization needed by the first RV64 and AArch64 children.
- Keep target ELF configuration explicit enough for later architecture-specific
  children to supply machine, class, endianness, flags, and relocation numbers.
- Add readelf-style or parser-backed structural checks where available.
- Avoid implementing target instruction encoding or target route integration.

Completion check:

- A minimal module serializes to relocatable ELF with expected sections,
  symbols, and relocations under focused tests.

## Step 5: Validate Scope And Handoff To Target Children

Goal: prove the shared API is ready for RV64 and AArch64 object-emission
children without absorbing their implementation work.

Actions:

- Run the focused object-model/ELF writer tests and a fresh build.
- Run existing asm-route or backend tests if touched surfaces can affect them.
- Update `todo.md` with proof commands, remaining known limitations, and the
  next child dependency notes.
- Do not add `--codegen obj`, target object emission, or c-testsuite route
  switching in this step.

Completion check:

- The source idea acceptance expectations are satisfied for the shared model
  and writer API, existing asm-route coverage remains meaningful, and the
  supervisor can ask the plan owner to close child 330 or switch to the next
  child.
