# c4c-objdump RV64 Custom Roundtrip

## Goal

Implement the first real `c4c-objdump` path for c4c-produced RV64 relocatable
objects and prove a stable custom-instruction roundtrip:

```text
source.c -> c4cll --codegen obj -> a.o
a.o -> c4c-objdump -> a.s
a.s -> c4c-as -> b.o
b.o -> c4c-objdump -> b.s
assert a.s == b.s
```

The purpose is not to recover C/C++ source. It is to avoid `<unknown>` for the
c4c custom 64-bit instruction stream and to keep `.s -> .o -> .s` stable.

## Dependency

This idea depends on:

- `ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md`
- `ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md`

Do not start this before `c4c-as` can assemble the canonical `.s` subset.

## Scope

First supported object/disassembly subset:

- c4c-produced RV64 relocatable ELF objects
- `.text` section extraction
- global function symbols such as `main`
- decoding:
  - c4c EV64 `.insn.d` 64-bit custom instruction
  - `li a0, 0` in the currently emitted simple shape
  - `ret`
- canonical `.s` printing:

  ```asm
  .text
  .globl main
  main:
    .insn.d 10, 11, v6, v0, v2, v4, 3
    li a0, 0
    ret
  ```

## In Scope

- ELF reader sufficient for c4c-produced relocatable objects.
- RV64 decoder for the initial subset.
- EV64 `.insn.d` decoder matching the encoder layout.
- Canonical assembly printer compatible with `c4c-as`.
- End-to-end roundtrip tests.

## Out Of Scope

- Full objdump clone behavior.
- Disassembly of arbitrary system objects.
- DWARF, source mapping, C/C++ reconstruction, or comments.
- Relocation expression pretty-printing beyond fail-closed diagnostics.
- External `llvm-objdump` compatibility for EV64 custom instructions.

## Acceptance Criteria

- `c4c-objdump input.o -o output.s` prints canonical `.s` for the current RV64
  VRM `.insn.d` object case.
- The printed `.s` assembles with `c4c-as`.
- Repeated roundtrip stabilizes:

  ```text
  a.o -> a.s -> b.o -> b.s
  a.s == b.s
  ```

- EV64 custom instruction bytes decode to `.insn.d` rather than `<unknown>`.
- Unknown or unsupported instruction bytes fail closed or print an explicit
  unsupported marker according to the runbook contract; they must not be
  silently misdecoded.

## Reviewer Reject Signals

- The decoder recognizes only one hard-coded byte string instead of decoding
  fields.
- The printer emits syntax that `c4c-as` cannot reassemble.
- The roundtrip uses external objdump/as as the source of truth.
- Unsupported bytes are silently skipped.

## Closure Note

Closed after proving the supported c4c-produced RV64 relocatable object route:
`c4c-objdump` prints canonical `.s`, `c4c-as` reassembles it, repeated
`a.o -> a.s -> b.o -> b.s` roundtrip output is stable, EV64 `.insn.d` bytes
decode explicitly, and unsupported bytes fail closed.
