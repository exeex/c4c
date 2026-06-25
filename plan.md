# c4c-as File Parser Using RV64 Line Core

Status: Active
Source Idea: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Activated after: ideas/closed/349_rv64_single_line_assembler_core_for_inline_asm.md

## Purpose

Implement the first real `c4c-as` path for a small RV64 `.s` file surface. The
file parser owns directives, labels, and symbol layout; every instruction line
must be parsed and encoded through the RV64 line core from idea 349.

Goal: `c4c-as input.s -o output.o` emits a relocatable RV64 ELF object whose
`.text` bytes match the current source-level VRM `.insn.d` object route.

## Core Rule

Do not reimplement `.insn.d`, `li`, or `ret` operand parsing in the file parser.
Instruction handling must call `parse_rv64_asm_line(...)` and
`encode_rv64_asm_line(...)`.

## Read First

- `ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md`
- `src/apps/c4c-as.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- existing backend object route tests around
  `backend_cli_riscv64_vrm_insn_d_source_obj`

## Current Targets

Supported file subset:

- `.text`
- `.globl <symbol>`
- `<label>:`
- blank lines and comments
- instruction lines accepted by the RV64 line core:
  - `.insn.d ...`
  - `li ...`
  - `ret`

Output target:

- relocatable RV64 ELF object using the existing c4c object model/writer route
- one global function symbol such as `main` with the correct text range

## Non-Goals

- macro expansion, includes, options, attributes, arbitrary sections, data/bss,
  alignment directives, relocation expressions, or linker relaxation
- full GNU assembler compatibility
- register allocation or inline asm constraints
- object disassembly

## Working Model

```text
c4c-as input.s -o output.o
  -> parse file directives and labels
  -> for each instruction line:
       parse_rv64_asm_line(...)
       encode_rv64_asm_line(...)
  -> build RiscvObjectFunction / ObjectModule
  -> write RV64 relocatable ELF
```

## Execution Rules

- Fail closed on unsupported directives, duplicate or misplaced labels, missing
  `.text`, malformed `.globl`, unknown instructions, and line-core parse or
  encode failures.
- Keep diagnostics clear enough for a CLI test to assert failure.
- Preserve the canonical bytes:
  `0a0320080b0300001305000067800000`.
- Use repo-native object tests; do not rely on external `as` for EV64 custom
  instruction encoding.

## Steps

### Step 1: Locate Existing c4c-as Placeholder And Object Writer Surfaces

Goal: identify the CLI placeholder, build wiring, and reusable RV64 object
emission APIs before implementing parsing.

Actions:

- inspect `src/apps/c4c-as.cpp` and top-level app build wiring
- inspect `RiscvObjectFunction`, `RiscvEncodedFragment`,
  `build_rv64_text_object_module(...)`, and
  `write_rv64_relocatable_elf_object(...)`
- identify the narrow object-route test command for `c4c-as`
- record owned files and proof command in `todo.md`

Completion check:

- `todo.md` names the first code packet and exact proof command

### Step 2: Add Minimal RV64 Assembly File Parser

Goal: parse the supported `.s` file subset into a simple file-level model.

Actions:

- read input text from `c4c-as <input.s> -o <output.o>`
- support blank lines and comments
- support exactly the needed `.text`, `.globl <symbol>`, and `<label>:` forms
- collect instruction lines without parsing operands locally
- reject unsupported directives, malformed labels, and instruction lines outside
  `.text`
- add focused parser/CLI failure tests if practical

Completion check:

- valid canonical input reaches instruction-line collection
- unsupported file-level syntax fails non-zero with a clear diagnostic

### Step 3: Assemble Instructions Through The RV64 Line Core

Goal: turn collected instruction lines into RV64 bytes using the line core.

Actions:

- call `parse_rv64_asm_line(...)` for each instruction line
- call `encode_rv64_asm_line(...)` for each parsed instruction
- append bytes into a single text fragment
- reject line-core parse or encode failures without silently skipping lines

Completion check:

- the canonical three-instruction file produces text bytes
  `0a0320080b0300001305000067800000`

### Step 4: Build And Write The Relocatable RV64 Object

Goal: emit an ELF object for the parsed file model.

Actions:

- create one `RiscvObjectFunction` from the supported global label
- preserve `.globl main` and `main:` as the symbol/function boundary
- call `build_rv64_text_object_module(...)`
- call `write_rv64_relocatable_elf_object(...)`
- write the image bytes to `-o`

Completion check:

- `c4c-as canonical.s -o output.o` writes an RV64 relocatable ELF object
- the object contains the expected `.text` bytes

### Step 5: Source-Route Equivalence And Failure Tests

Goal: prove `c4c-as` output matches the corresponding `c4cll --codegen obj`
source-level inline asm case.

Actions:

- add a CTest case for the canonical `.s` input
- compare the produced object bytes or `.text` hex against the existing
  source-level VRM `.insn.d` object route
- add a negative test for unsupported file syntax

Completion check:

- object equivalence proof passes
- unsupported syntax fails non-zero

### Step 6: Broader Validation And Closure Review

Goal: decide whether idea 350 is complete and ready for dependent objdump work.

Actions:

- run the supervisor-selected broader backend/object validation subset
- inspect the diff for duplicate instruction parsing or silent skipping
- confirm idea 351 can rely on `c4c-as` for canonical `.s` reassembly

Completion check:

- validation logs are recorded in canonical proof files
- no reviewer reject signal from the source idea applies
- dependent idea 351 can start
