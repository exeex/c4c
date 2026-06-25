# c4c-as File Parser Using RV64 Line Core

## Goal

Implement the first real `c4c-as` path by parsing a small RV64 `.s` file
surface and assembling each instruction line through the RV64 single-line
assembler core from idea 349.

`c4c-as` owns file syntax. It must not duplicate target instruction parsing or
encoding logic.

## Dependency

This idea depends on:

- `ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md`

Do not start this before the RV64 line parser/encoder exists and inline asm
uses it.

## Scope

First supported `.s` file subset:

- `.text`
- `.globl <symbol>`
- `<label>:`
- blank lines and comments
- instruction lines accepted by the RV64 line core:
  - `.insn.d ...`
  - `li ...`
  - `ret`

The output should be a relocatable RV64 ELF object using the existing c4c object
writer route.

## Desired Flow

```text
c4c-as input.s -o output.o
  -> parse file-level directives and labels
  -> for each instruction line:
       parse_rv64_asm_line(...)
       encode_rv64_asm_line(...)
  -> build object model
  -> write RV64 relocatable ELF
```

## In Scope

- CLI argument handling for `c4c-as <input.s> -o <output.o>`.
- Minimal file parser for the supported directives and labels.
- `.text` section emission.
- One global function symbol such as `main`, with correct text range.
- Direct object bytes equivalent to `c4cll --codegen obj` for the current
  source-level VRM `.insn.d` case.
- Diagnostics/fail-closed behavior for unsupported directives or malformed
  lines.

## Out Of Scope

- Macro expansion, `.include`, `.option`, `.attribute`, `.section`, `.data`,
  `.bss`, alignment directives, relocation expressions, or linker relaxation.
- Full GNU assembler compatibility.
- Register allocation or inline asm constraints.
- Object disassembly.

## Acceptance Criteria

- `c4c-as` can assemble a canonical `.s` file containing:

  ```asm
  .text
  .globl main
  main:
    .insn.d 10, 11, v6, v0, v2, v4, 3
    li a0, 0
    ret
  ```

- The resulting object has the same `.text` bytes as the corresponding
  `c4cll --codegen obj` source-level inline asm case.
- Unsupported file-level syntax fails with a clear diagnostic and non-zero
  exit status.
- `c4c-as` instruction handling calls the RV64 line core from idea 349 rather
  than parsing instruction operands independently.

## Reviewer Reject Signals

- The file parser reimplements `.insn.d`, `li`, or `ret` parsing separately
  from the line core.
- The assembler silently ignores unknown directives or instructions.
- The proof relies on external `as` for custom instruction encoding.
- Symbol size or `.text` bytes differ from the c4c object route without an
  explicit reason.
