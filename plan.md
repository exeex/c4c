# Built-in x86 Assembler Runbook

Status: Active
Source Idea: ideas/open/23_backend_builtin_assembler_x86_plan.md

## Purpose

Activate the first built-in x86 assembler slice so the compiler can emit working ELF relocatable objects for the currently supported x86 backend subset without relying on an external assembler for that bounded path.

## Goal

Compile and wire the mirrored x86 assembler subtree, support the minimum instruction and relocation subset already exercised by current x86 backend tests, and validate emitted `.o` files against externally assembled references.

## Core Rule

Keep this slice narrow and mechanical: follow the ref assembler structure, port only what the current x86 backend output requires, and do not expand into linker work or wider instruction-set coverage.

## Read First

- [ideas/open/23_backend_builtin_assembler_x86_plan.md](/workspaces/c4c/ideas/open/23_backend_builtin_assembler_x86_plan.md)
- [ref/claudes-c-compiler/src/backend/x86/assembler/README.md](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/assembler/README.md)
- [ref/claudes-c-compiler/src/backend/x86/assembler/mod.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/assembler/mod.rs)
- [ref/claudes-c-compiler/src/backend/x86/assembler/parser.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/assembler/parser.rs)
- [ref/claudes-c-compiler/src/backend/x86/assembler/elf_writer.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/assembler/elf_writer.rs)
- [ref/claudes-c-compiler/src/backend/x86/assembler/encoder/](/workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/assembler/encoder/)

## Current Targets

- `src/backend/x86/assembler/`
- shared ELF support the assembler depends on
- the assembler entry boundary chosen for the x86 backend path
- validation fixtures that compare generated objects against external assembly output

## Non-Goals

- x86 linker implementation
- full x86 instruction-set coverage
- widening backend codegen scope beyond the currently exercised subset
- non-x86 backend work

## Working Model

1. Identify the exact x86 assembly emitted by current backend tests.
2. Port only the parser and encoder surfaces needed for that output.
3. Port object-writing support around shared ELF helpers.
4. Route one minimal backend-emitted function through the built-in assembler path.
5. Validate object layout, relocations, and disassembly against externally assembled references.

## Execution Rules

- Treat the ref backend layout as the default shape unless the existing C++ backend forces a small adapter.
- Prefer the smallest testable slice and add or update validation before broadening support.
- If execution uncovers a new backend feature requirement outside this subset, record it under `ideas/open/` instead of absorbing it here.
- Keep linker concerns out of this runbook; unresolved linker follow-on stays with idea `24`.

## Ordered Steps

### Step 1: Inspect the current x86 backend output subset

Goal: establish the exact assembler surface required by existing x86 backend tests.

Primary target: current x86 backend tests and emitted assembly/object expectations.

Concrete actions:

- inventory the x86 backend tests that already emit or expect x86 assembly/object output
- capture the current assembly forms, directives, symbols, and relocations those tests require
- map the required subset back to the ref assembler parser, encoder, and writer surfaces

Completion check:

- there is a bounded list of required instructions, directives, symbol forms, and relocations for the first slice

### Step 2: Port the minimum parser surface

Goal: make the assembler accept the bounded backend-emitted x86 assembly subset.

Primary target: `src/backend/x86/assembler/` parser-facing files.

Concrete actions:

- mirror only the parser entry points and data structures needed for the bounded subset
- keep unsupported syntax explicit rather than silently widening coverage
- add focused tests that exercise accepted and rejected forms for the first slice

Completion check:

- the parser compiles and can represent all assembly constructs needed by the bounded subset

### Step 3: Port the minimum encoder surface

Goal: encode the required x86 instructions and operand forms for the current backend subset.

Primary target: `src/backend/x86/assembler/encoder/`

Concrete actions:

- port the smallest encoder slices matching the Step 1 inventory
- preserve ref naming and mechanism boundaries where practical
- add or update tests that check bytes, fixups, and failure paths for unsupported encodings

Completion check:

- the encoder compiles and emits correct bytes/fixups for the bounded subset

### Step 4: Port ELF object writing support

Goal: emit working ELF relocatable objects for the supported assembler subset.

Primary target: assembler writer code plus shared ELF helpers it depends on.

Concrete actions:

- port the writer path needed to produce relocatable ELF objects
- wire symbols, sections, and relocations only for the supported slice
- keep shared ELF support changes mechanical and scoped to assembler requirements

Completion check:

- the assembler path can write `.o` files for representative supported inputs

### Step 5: Wire one backend-emitted function through the built-in assembler path

Goal: prove end-to-end integration for the first bounded backend-owned path.

Primary target: the chosen assembler entry boundary from the x86 backend.

Concrete actions:

- route one minimal x86 backend-emitted function through the built-in assembler path
- keep external assembler fallback behavior intact for unsupported cases if that exists today
- add or update a focused integration test covering the routed path

Completion check:

- one minimal backend-emitted function successfully produces an object through the built-in assembler path

### Step 6: Validate against external assembler output

Goal: confirm object correctness for the supported slice before widening coverage.

Primary target: object comparison and disassembly validation.

Concrete actions:

- compare generated `.o` files against externally assembled references for representative cases
- check sections, symbols, relocations, and disassembly
- run targeted tests, nearby backend tests, and the full regression suite

Completion check:

- supported cases produce comparable object metadata and correct disassembly with no regression in the existing suite
