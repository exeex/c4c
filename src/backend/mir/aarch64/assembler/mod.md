# AArch64 Assembler Module Legacy Surface

This artifact preserves the useful structure from the removed
`mod.cpp` translation unit. The old file was the orchestration layer for the
legacy AArch64 assembler lane: it parsed assembly text, expanded literal pools,
resolved GNU-style numeric labels, and optionally called the local ELF writer.

## Role

The removed module implemented the public surface declared by `mod.hpp`:

- `expand_literal_pools(const std::vector<AsmStatement>& statements)
  -> std::vector<AsmStatement>`
- `resolve_numeric_labels(const std::vector<AsmStatement>& statements)
  -> std::vector<AsmStatement>`
- `assemble(const AssembleRequest& request) -> AssembleResult`
- `assemble(const std::string& asm_text, const std::string& output_path)
  -> std::string`

It also declared the assembler-local object-writer hook:

- `write_elf_object(const std::vector<AsmStatement>& statements,
  const std::string& output_path) -> bool`

That writer was implemented by the old `elf_writer.cpp` surface and is now
recorded separately in `elf_writer.md`.

## Data Shapes Recorded In The Old Surface

The file carried two local helper shapes:

- `NumericDefs`: a vector of statement-index and replacement-label pairs for
  one numeric label spelling
- `PoolEntry`: generated literal-pool label, referenced symbol, and signed
  addend

It also used a fixed literal-pool alignment constant of 8 bytes and emitted
pool labels with the `.Llpool_` prefix.

## Literal-Pool Expansion

`expand_literal_pools` walked parsed statements and rewrote pseudo-load
operands of the form:

```text
ldr <register>, =<symbol-or-symbol-plus-addend>
```

For each match, the old module:

1. split the expression after `=` into a symbol and optional decimal addend
2. generated a private `.Llpool_N` label
3. replaced the instruction operand with that generated pool label
4. queued a pool entry that later emitted a label plus `.quad <symbol>` or
   `.quad <symbol><signed-addend>`

The queued pool was flushed before section-changing directives and at the end
of the statement stream. A flush emitted `.balign 8`, then one generated label
and one `.quad` directive per queued entry.

The section-changing/directive flush set included `.text`, `.data`, `.rodata`,
`.bss`, `.section`, `.pushsection`, `.popsection`, `.previous`, `.subsection`,
and `.ltorg`.

## Symbol Addend Handling

The old `split_symbol_addend` helper accepted only simple decimal addends in
expressions like `sym+8` and `sym-16`. It ignored leading `+` or `-` when
searching for the split point so signed symbol names or malformed expressions
fell back to the whole input as a symbol with zero addend.

Hexadecimal, parenthesized, nested, or symbolic addends were not parsed by this
module. They remained as raw symbol text when the helper could not recognize
the expression.

## Numeric-Label Resolution

`resolve_numeric_labels` implemented GNU-style local numeric labels for the
backend-owned statement stream:

- numeric definitions like `1:` were replaced with unique labels such as
  `.Lnum_1_0`
- forward references like `1f` and `1F` resolved to the next matching numeric
  label definition after the current statement
- backward references like `1b` and `1B` resolved to the previous matching
  numeric label definition before the current statement

The resolver rewrote:

- numeric label statements into generated `.Lnum_<number>_<ordinal>` labels
- instruction operand text and reconstructed `raw_operands`
- directive `raw_operands`, operand text, reconstructed `raw_operands`, and
  reconstructed directive `text`

Numeric-looking literals with `0x`, `0X`, `0b`, or `0B` prefixes were preserved
as literals instead of being treated as label references.

## Assembly Entry Point

`assemble(const AssembleRequest&)` chained the legacy stages in this order:

1. `parse_asm(request.asm_text)`
2. `expand_literal_pools(parsed)`
3. `resolve_numeric_labels(expanded)`
4. `write_elf_object(resolved, request.output_path)` when `output_path` was
   non-empty

The returned `AssembleResult` preserved the original input text in
`staged_text`, copied the output path, and reported whether object emission
returned true. It did not return the expanded or resolved statement stream and
did not surface structured diagnostics.

The string overload was a compatibility seam. It called the request-based
entry point and returned only `staged_text`.

## Dependencies And Entry Points

The removed translation unit depended on:

- `mod.hpp` for the public assembler request/result contract
- `parser.hpp` and `types.hpp` through `mod.hpp` for `AsmStatement`,
  `AsmStatementKind`, `Operand`, and `parse_asm`
- the private `write_elf_object` entry point from the old ELF writer surface
- standard library strings, vectors, hash maps, pairs, streams, and fixed-width
  integer types

The module did not encode instructions itself. It prepared statements for the
ELF writer, which then integrated with the old encoder surface.

## Hidden Assumptions

- Input assembly was already close to the backend's own emitted GNU-style text.
- Literal-pool pseudo loads used a simple `ldr` mnemonic and an operand whose
  first character was `=`.
- Literal-pool expressions were symbol names with optional decimal addends.
- Flushing pools before broad section-changing directives was sufficient to
  keep generated data reachable and in the intended section.
- Numeric label references appeared in plain operand or directive text, not in
  forms that required a full expression parser.
- Reconstructing raw operand strings with comma-space separators was acceptable
  after rewriting references.
- A boolean from `write_elf_object` was enough to describe object-emission
  success or failure.

## Failure Risks For Rebuild

- Literal-pool reachability was not range-checked. A rebuild must account for
  branch/load reach, section boundaries, and placement policy explicitly.
- The `ldr =expr` recognizer was shape-based and did not validate target
  register class, literal form, or instruction encoding constraints.
- The addend splitter accepted only decimal integer suffixes and could lose
  meaning for richer assembler expressions.
- Numeric-reference rewriting operated directly on text and may rewrite tokens
  inside forms that a real parser would treat differently.
- Directive reconstruction may change whitespace and formatting even when only
  one numeric operand changed.
- The assembly API returned the original input text, so callers could not
  observe expanded pools or resolved labels except through object emission.
- Object emission failure collapsed parser, expansion, label resolution,
  encoding, relocation, layout, and I/O failures into one boolean.

## Rebuild Guidance

Use this artifact as the historical contract for the assembler orchestration
lane. A new live module should keep parsing, pseudo-instruction lowering,
numeric-label resolution, object writing, and diagnostics as explicit stages
with inspectable intermediate state. Literal-pool placement and numeric labels
should be implemented through structured operands or expressions rather than
raw text rewrites.
