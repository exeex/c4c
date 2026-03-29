# Built-in x86 Linker Runbook

Status: Active
Source Idea: ideas/open/24_backend_builtin_linker_x86_plan.md

## Purpose

Activate the first built-in x86 linker slice so the compiler can link a bounded
set of already-supported x86 objects into working ELF executables without
relying on the external linker for that narrow path.

## Goal

Compile and wire the mirrored x86 linker subtree, support the minimum shared
input and relocation subset exercised by the first simple x86 linker tests, and
validate emitted executables against externally linked references.

## Core Rule

Keep this linker slice narrow and mechanical: follow the ref linker structure,
support only the first bounded static-executable path, and do not broaden into
dynamic-linking or wide relocation coverage.

## Read First

- [ideas/open/24_backend_builtin_linker_x86_plan.md](/workspaces/c4c/ideas/open/24_backend_builtin_linker_x86_plan.md)
- [ideas/open/__backend_port_plan.md](/workspaces/c4c/ideas/open/__backend_port_plan.md)
- [ref/claudes-c-compiler/src/backend/x86/linker/README.md](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/linker/README.md)
- [ref/claudes-c-compiler/src/backend/x86/linker/mod.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/linker/mod.rs)
- [ref/claudes-c-compiler/src/backend/x86/linker/link.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/linker/link.rs)
- [ref/claudes-c-compiler/src/backend/x86/linker/input.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/linker/input.rs)
- [ref/claudes-c-compiler/src/backend/x86/linker/elf.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/linker/elf.rs)
- [ref/claudes-c-compiler/src/backend/x86/linker/emit_exec.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/linker/emit_exec.rs)

## Current Targets

- `src/backend/x86/linker/`
- `src/backend/linker_common/`
- `src/backend/elf/`
- x86 linker-facing backend and fixture tests
- bounded static executable emission for simple x86 objects

## Non-Goals

- dynamic linking, shared-library output, or PLT/GOT polish beyond what the
  first static slice strictly needs
- full relocation coverage
- new assembler or codegen feature work
- non-x86 linker work

## Working Model

1. Identify the smallest x86 linker-owned executable path already representable
   by current fixtures and backend-owned objects.
2. Port only the shared input/loading and x86 relocation/emission surfaces
   needed for that path.
3. Emit one bounded static ELF executable for the first supported case.
4. Validate executable metadata and simple runtime behavior against the external
   linker path before broadening coverage.

## Execution Rules

- Treat the ref linker layout as the default shape unless the existing C++
  shared linker layers force a small adapter.
- Prefer the smallest multi-object or relocation-bearing slice that can be
  tested end-to-end.
- Keep shared parsing and symbol-registration changes mechanical and scoped to
  x86 linker requirements.
- If execution uncovers a separate linker initiative, write it into
  `ideas/open/` instead of silently widening this runbook.
- Do not absorb assembler-expansion work here unless it is strictly required to
  finish the bounded linker slice.

## Ordered Steps

### Step 1: Inspect the first bounded x86 linker slice

Goal: establish the exact object shapes, relocations, and executable properties
required for the first linker-owned path.

Primary target: current x86 linker tests, shared linker fixtures, and bounded
backend-owned x86 objects.

Concrete actions:

- inventory current x86 linker tests, fixtures, and helper seams
- identify the smallest relocation-bearing or multi-object slice that should be
  linked first
- map required sections, symbols, and relocation types back to ref/shared/x86
  linker surfaces

Completion check:

- there is a bounded list of object inputs, relocation types, and executable
  properties for the first linker slice

### Step 2: Compile-integrate the minimum x86 linker orchestration surface

Goal: make the x86 linker subtree compile and expose the first linker entry
boundary.

Primary target: `src/backend/x86/linker/`

Concrete actions:

- port the smallest `mod.rs` / `link.rs` style entry surfaces needed for static
  linking
- keep unsupported operations explicit rather than widening feature coverage
- add or update focused tests that compile the linker-facing contract surface

Completion check:

- the x86 linker entry boundary compiles and can accept the bounded input set

### Step 3: Port the bounded shared input and symbol-loading path

Goal: load the first supported x86 object/archive inputs through the shared
  linker layers.

Primary target: `src/backend/linker_common/` helpers touched by the x86 linker
and x86-specific input glue.

Concrete actions:

- reuse shared object/archive parsing wherever available
- wire only the symbol registration and input loading needed for the bounded
  slice
- add focused tests covering loaded sections, symbols, and archive/object
  membership for the chosen inputs

Completion check:

- the x86 linker can load and classify the bounded input objects it needs

### Step 4: Implement the minimum x86 relocation and resolution slice

Goal: resolve symbols and apply only the relocation subset required by the
first executable path.

Primary target: `src/backend/x86/linker/elf.cpp`, `link.cpp`, and closely
related types/helpers.

Concrete actions:

- port the smallest relocation and symbol-resolution logic required by the Step
  1 inventory
- keep unsupported relocation kinds explicit
- add or update tests that check patched bytes, symbol resolution, and failure
  paths for out-of-scope relocations

Completion check:

- the bounded relocation-bearing or multi-object slice links correctly in memory

### Step 5: Emit one bounded static x86 executable

Goal: produce one working ELF executable for the first supported x86 linker
path.

Primary target: x86 executable emission code and the chosen linker entry seam.

Concrete actions:

- port the minimal static executable emitter surface needed by the bounded slice
- keep layout, headers, and section/program-header emission scoped to the first
  supported case
- add or update an integration test covering the emitted executable image

Completion check:

- one bounded x86 program links into an ELF executable through the built-in
  linker path

### Step 6: Validate against the external linker path

Goal: confirm executable correctness for the supported slice before widening
coverage.

Primary target: executable comparison and runtime validation.

Concrete actions:

- compare the built-in linked executable against an externally linked reference
  for representative supported cases
- check executable layout, symbol surface, and runtime behavior
- run targeted tests, nearby linker tests, and the full regression suite

Completion check:

- supported cases produce comparable executable behavior and no regression in
  the existing suite
