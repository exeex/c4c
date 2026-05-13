# AArch64 Encoder Module Legacy Surface

This artifact preserves the useful structure from the removed
`mod.cpp` translation unit. The old file was the assembler encoder module
surface behind `mod.hpp`: shared register/immediate helpers plus the top-level
`encode_instruction` dispatch entry point.

## Role

The removed surface was not a full AArch64 assembler encoder. It was a small
compiled C++ stub that satisfied the module interface while only recognizing a
very narrow instruction subset:

- `ret` with no operands
- `mov wN, #imm16` as a 32-bit move-wide-immediate word

All other mnemonics returned an unencoded default `EncodeResult`. Most helper
functions were placeholders that returned zero-valued defaults.

## Public Entry Points

`mod.hpp` exposed these functions from the removed implementation:

- `parse_reg_num(name)`
- `is_64bit_reg(name)`
- `is_32bit_reg(name)`
- `is_fp_reg(name)`
- `encode_cond(cond)`
- `encode_instruction(mnemonic, operands, raw_operands)`
- `get_reg(operands, idx)`
- `get_imm(operands, idx)`
- `get_symbol(operands, idx)`
- `sf_bit(is_64)`

The old file also had private helpers:

- `parse_unsigned(text)`
- `parse_w_register(text)`
- `parse_mov_immediate(text)`

## Register And Condition Helpers

The register classification helpers were string-prefix checks only:

- `is_64bit_reg` accepted any non-empty name beginning with `x`
- `is_32bit_reg` accepted any non-empty name beginning with `w`
- `is_fp_reg` accepted any non-empty name beginning with `d`, `s`, `q`, or `v`

`parse_reg_num` ignored its input and returned zero. `encode_cond` also ignored
its input and returned zero. Rebuilding this surface should not treat either
function as an architectural parser; they were placeholders.

`sf_bit(is_64)` was the only complete shared helper. It returned `1` for
64-bit forms and `0` otherwise.

## Operand Extraction Helpers

The old `get_reg`, `get_imm`, and `get_symbol` helpers did not inspect
operands:

- `get_reg` returned `{0, false}`
- `get_imm` returned `0`
- `get_symbol` returned `{"", 0}`

Other extracted encoder-family artifacts describe callers that expected these
helpers to parse `Operand` values. A future rebuild needs real operand
extraction semantics before those historical encoders can be made live.

## Immediate Parsing

The private decimal parser accepted only ASCII digits and rejected empty input.
It accumulated into `std::uint32_t` without overflow diagnostics.

`parse_w_register` accepted names of the form `w<number>` and required the
number to be at most 31.

`parse_mov_immediate` accepted immediates of the form `#<number>` and required
the value to be at most 65535.

Negative values, hexadecimal syntax, symbolic constants, aliases, shifted
immediates, and 64-bit registers were outside the stub.

## Dispatch Surface

`encode_instruction` accepted the mnemonic, parsed operands, and raw operand
text. The raw text was ignored.

Recognized encodings were:

```text
ret -> 0xd65f03c0
mov wN, #imm16 -> 0x52800000 | (imm16 << 5) | Rd
```

Both recognized cases returned:

```text
encoded = true
kind = EncodeResultKind::Word
word = encoded instruction
```

Every other instruction returned the default `EncodeResult`, which left
`encoded = false` and `kind = EncodeResultKind::Skip`.

## Dependencies And Data Model

The surface depended on:

- `mod.hpp` for `EncodeResult`, `EncodeResultKind`, and public declarations
- `../types.hpp` through `mod.hpp` for `Operand`
- operand `.text` fields for the narrow `mov` parser
- `std::optional` for local parse failure propagation

It did not create relocations, multi-word encodings, labels, symbol references,
or diagnostic information.

## Structured Rebuild Boundary

A rebuilt encoder may preserve these historical opcode and field-packing facts,
but its codegen-facing input must be structured AArch64 machine instruction
nodes or lower structured encoding records. `mnemonic`, `Operand::text`, and
`raw_operands` are acceptable only on an external assembler-input lane after a
real assembler parser has produced diagnostics; they are not the semantic
handoff from codegen to encoding or object writing.

## Rebuild Risks

- The file name suggests central dispatch, but the removed implementation was
  only a stub. Rebuilding by preserving its behavior would leave most AArch64
  assembler coverage absent.
- The helper names are shared across the historical encoder modules. A future
  implementation should define precise `Operand` contracts before making
  `get_reg`, `get_imm`, or `get_symbol` authoritative.
- `encode_cond` returning zero would silently map every condition to `eq` if
  used by real conditional encoders. It must be replaced by an explicit
  condition-code table.
- The `mov` encoding only covered a single 16-bit `MOVZ`-style 32-bit form and
  did not handle aliases, shifts, 64-bit destinations, negative constants, or
  wide-immediate decomposition.
- Unsupported instructions were skipped without diagnostics, which is unsafe
  for a production assembler path.
