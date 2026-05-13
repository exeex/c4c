# AArch64 Assembler Parser Legacy Surface

This artifact preserves the useful structure from the removed
`parser.cpp` translation unit. The old file was a small text parser for the
legacy AArch64 assembler lane, not a complete AArch64 assembly grammar.

## Role

The removed parser converted raw assembly text into `AsmStatement` records for
the assembler pipeline. Its public surface was declared by `parser.hpp`:

- `trim_asm(std::string_view text) -> std::string`
- `parse_asm(const std::string& text) -> std::vector<AsmStatement>`

The parser produced line-oriented statements. It classified each non-empty
trimmed line as one of:

- `Label` when the full trimmed line ended in `:`
- `Directive` when the parsed operation began with `.`
- `Instruction` otherwise

## Data Shapes Recorded In The Old Surface

The file carried several local record types that described intended parser
concepts but were not wired into the implemented line parser:

- `SymbolKind`: function, object, TLS object, or no-type symbol
- `SectionDirective`: section name plus optional flags and section type
- `SizeExpr`: either a constant or current-location-minus-symbol expression
- `DataValue`: integer, symbol, symbol plus offset, symbol difference,
  symbol difference plus addend, or raw expression
- `AsmDirective`: raw directive text

These shapes show the expected assembler-domain vocabulary for later rebuilds.
They were not exposed by `parser.hpp` and did not drive the actual
`parse_asm` result in the removed C++.

## Parsing Behavior

`trim_asm` removed leading and trailing spaces, tabs, and carriage returns.
It did not trim other whitespace classes and did not strip comments.

`parse_operands` split the raw operand tail on every comma, trimmed each
piece with `trim_asm`, discarded empty pieces, and stored each surviving piece
as `Operand{text}`. It did not understand nested expressions, quoted strings,
bracketed addressing modes, or comma-bearing directive payloads.

`parse_asm` scanned the input by newline. For each non-empty trimmed line:

1. a trailing colon made the line a label; the label text retained the colon
   in `text`, while `op` stored the label name without the colon
2. otherwise, the first space or tab separated `op` from `raw_operands`
3. operations beginning with `.` became directives
4. all other operations became instructions
5. operands were the comma-split result of `raw_operands`

The parser preserved the original trimmed statement in `AsmStatement::text`
and the unparsed operand tail in `AsmStatement::raw_operands`.

## Dependencies And Entry Points

The removed translation unit depended on:

- `parser.hpp` for the public declarations
- `types.hpp` indirectly through `parser.hpp` for `AsmStatement`,
  `AsmStatementKind`, and `Operand`
- standard library containers, strings, integer types, and optional/variant
  helpers used by the local legacy shapes

Downstream assembler code treated `parse_asm` as the text-to-statement entry
point before literal-pool expansion and object-writer staging. The parser did
not encode instructions, resolve symbols, expand literal pools, or emit ELF.

## Hidden Assumptions

- The incoming assembly was already close to the backend's own emitted style.
- One physical line represented one statement.
- A label line contained only the label and trailing colon.
- Operand commas were structural separators, never part of nested syntax.
- Comments were either absent or tolerated as part of the final operand text.
- Directives could be classified by a leading `.` without directive-specific
  parsing.
- Parser success was unconditional; malformed lines still became statements.

## Failure Risks For Rebuild

- Reusing the comma splitter for full AArch64 syntax would misparse addressing
  forms, macro-like directives, quoted strings, and expression lists.
- Treating this parser as validation would let invalid assembly flow into
  later assembler stages because the old parser reported no parse errors.
- Preserving only `op` and raw operand text leaves directive-specific meaning,
  relocation expressions, symbol attributes, and section flags to later stages.
- The unused local record types hint at intended richer directive handling; a
  rebuild should either implement that structure deliberately or remove it from
  the design.
- The parser's line model cannot represent multi-line assembler constructs
  without a new stateful parse layer.

## Rebuild Guidance

Use this artifact as the legacy contract for the parser lane: a lightweight
normalizer from backend-owned assembly text into `AsmStatement` values. A new
live parser should define which syntax it accepts, where diagnostics are
reported, and which directive/expression forms are parsed structurally before
object emission.
