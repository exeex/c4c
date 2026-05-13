# AArch64 Assembler ELF Writer Legacy Surface

This artifact preserves the useful structure from the removed
`elf_writer.cpp` translation unit. The old file was a target-local ELF object
writer for the legacy AArch64 assembler lane. It converted parsed
`AsmStatement` values into a best-effort relocatable ELF image and wrote the
bytes to disk.

## Role

The removed writer exposed one assembler-local entry point:

- `write_elf_object(const std::vector<AsmStatement>& statements,
  const std::string& output_path) -> bool`

The old assembler module declared this function privately in `mod.cpp` and
called it after parsing and literal-pool expansion when an output path was
requested. The writer owned directive interpretation, section byte
accumulation, symbol table staging, relocation staging, and final ELF64 section
layout for `EM_AARCH64` relocatable objects.

## Data Shapes Recorded In The Old Surface

The file carried several local staging records:

- `PendingReloc`: section name, byte offset, ELF relocation type, symbol, and
  addend for instruction relocations
- `PendingExpr`: unresolved expression placeholder with section, offset,
  expression text, and size
- `PendingSymDiff`: symbolic difference placeholder with two symbols, extra
  addend, size, and output location
- `PendingInstruction`: unresolved instruction placeholder with mnemonic,
  parsed operands, raw operand text, and section/offset
- `Section`: logical assembler section with ELF type, flags, alignment, bytes,
  and relocations
- `Symbol`: legacy symbol record with binding, type, section index, value,
  defined/global state

`PendingExpr`, `PendingSymDiff`, and `PendingInstruction` had empty resolver
functions at the end of the file. They document intended rebuild vocabulary,
but the implemented object writer did not resolve those pending forms.

## Parsing And Staging Behavior

`parse_slice` walked the parsed assembler statements and built a `ParsedSlice`.
It started in `.text`, creating sections on demand. Built-in sections received
default flags and alignment:

- `.text`: allocated, executable, 4-byte alignment
- `.data` and `.rodata`: allocated, 4-byte alignment
- `.bss`: allocated, 4-byte alignment, `SHT_NOBITS`

The writer recognized `.section`, `.globl`/`.global`, `.type`, alignment
directives, zero-fill directives, integer data directives, and string
directives. Unknown directives caused the parse to fail. Instructions were sent
to `encoder::encode_instruction`; encoded words were appended to the active
section and encoder relocation kinds were converted to AArch64 ELF relocation
numbers.

The slice was rejected when no section data and no pending relocation existed.
Instructions in `SHT_NOBITS` sections were rejected.

## Directive Semantics

The old directive handling was intentionally narrow:

- `.section` selected or created a named section, optionally deriving flags
  from `a`, `x`, and `w` letters and detecting `nobits`/`SHT_NOBITS`
- `.globl` and `.global` marked existing symbols global or created undefined
  global symbols
- `.type` understood `%function`; other type text became no-type
- `.align` and `.balign` treated the numeric operand as a byte alignment
- `.p2align` treated the numeric operand as a power-of-two exponent
- `.space`, `.skip`, and `.zero` appended zero bytes
- `.byte`, `.short`, `.word`, `.int`, `.long`, and `.quad` accepted only
  numeric operands, except `.byte` could also append quoted string contents
- `.ascii` and `.asciz` appended unquoted string bytes, with `.asciz` adding a
  terminating NUL byte

Numeric parsing handled optional sign prefixes plus decimal and `0x`/`0X`
hexadecimal forms.

## ELF Emission Behavior

`build_elf_object` produced an ELF64 relocatable object with:

- ELF class 64, little-endian data encoding, `ET_REL`, and `EM_AARCH64`
- output sections copied from the parsed slice, plus generated `.rela.<name>`
  sections for any section with pending relocations
- local section symbols for output sections
- parsed symbols and undefined relocation targets in `.symtab`
- generated `.symtab`, `.strtab`, and `.shstrtab`
- 64-byte ELF and section headers and 24-byte symbol/RELA entries

Relocation entries encoded `r_offset`, `r_info`, and signed addend. The writer
mapped selected encoder relocation kinds to AArch64 ELF relocation numbers,
including call/jump branches, ADRP/ADD/LDST low-12 forms, GOT forms, TLS
fallbacks, conditional/test branches, ADR prelude, and absolute/prel
relocations. Unknown or unsupported relocation mappings returned zero and made
the slice fail.

The writer also exposed `is_branch_reloc_type`, returning true for AArch64 test
branch, conditional branch, jump26, and call26 relocation numbers.

## Dependencies And Entry Points

The removed translation unit depended on:

- `parser.hpp` for `AsmStatement`, `AsmStatementKind`, and `Operand`
- `encoder/mod.hpp` for `encode_instruction`, `EncodeResultKind`, relocation
  kinds, and encoded instruction payloads
- standard library strings, vectors, variants, optionals, file streams, and
  fixed-width integer types

The file wrote bytes directly with `std::ofstream` and did not report structured
diagnostics. A false return meant parsing, encoding, byte construction, file
open, or file write failure.

## Hidden Assumptions

- The incoming statement stream came from the backend-owned lightweight parser
  and matched the backend's own emitted assembly style.
- Section and symbol names were already valid for ELF string tables.
- Relocations emitted by the encoder were enough to represent all unresolved
  instruction operands.
- Unsupported directives, expressions, and relocation types could fail the
  entire object emission path rather than producing diagnostics.
- Endianness was fixed to little-endian ELF64 AArch64.
- `.bss` carried a logical size through its data vector even though bytes were
  not emitted for `SHT_NOBITS`.
- The local symbol partition was based on insertion order and the simple
  `local_symbol_end` counter.

## Failure Risks For Rebuild

- The section header `link`/`info` fields for RELA sections were assigned after
  output section headers were emitted in the old implementation. A rebuild must
  compute those values before serializing headers.
- The directive parser did not handle expression-valued data, symbolic
  constants, symbol differences, escapes, nested comma syntax, or rich section
  attributes.
- `PendingExpr`, `PendingSymDiff`, and `PendingInstruction` were placeholders
  with no resolution implementation; copying those names alone would not
  restore behavior.
- Returning `false` for every failure made debugging difficult and collapsed
  parse, encode, layout, and I/O failures into one result.
- Relocation mapping was partial and included best-effort TLS fallbacks and a
  zero value for unsupported `Ldr19`.
- The writer depended on the legacy text parser's simplistic operand splitting,
  so full AArch64 syntax or GNU assembler compatibility would need a stronger
  front end.

## Rebuild Guidance

Use this artifact as the historical contract for the assembler object-writer
lane: parse backend-owned assembly statements into sections, symbols,
relocations, and ELF64 object bytes. A new live writer should separate directive
semantics, encoder integration, relocation modeling, ELF layout, and diagnostic
reporting so unsupported syntax and invalid relocation states are explicit
rather than silent `false` results.
