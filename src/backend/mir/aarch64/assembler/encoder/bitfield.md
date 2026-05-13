# AArch64 Encoder Bitfield Legacy Surface

This artifact preserves the useful structure from the removed
`bitfield.cpp` translation unit. The old file was a commented C++ mirror of
the reference Rust encoder surface at
`ref/claudes-c-compiler/src/backend/arm/assembler/encoder/bitfield.rs`.

## Role

The removed surface documented how the legacy assembler encoder handled
AArch64 bitfield, extract, bit-manipulation, byte-reversal, and CRC32
mnemonics. It did not provide active compiled C++ behavior in this tree; each
routine was preserved as commented Rust-shaped pseudocode.

The surface expected instruction operands from the legacy assembler parser and
returned one `EncodeResult::Word` for every supported mnemonic form.

## Entry Points

The old surface listed these encoder entry points:

- `encode_ubfx(operands)`: alias for `UBFM` using `immr = lsb` and
  `imms = lsb + width - 1`
- `encode_sbfx(operands)`: alias for `SBFM` using the same extract range
  mapping
- `encode_ubfm(operands)`: raw unsigned bitfield move form with explicit
  `immr` and `imms`
- `encode_sbfm(operands)`: raw signed bitfield move form with explicit
  `immr` and `imms`
- `encode_sbfiz(operands)`: signed bitfield insert-in-zero alias using
  `immr = (-lsb mod regsize)` and `imms = width - 1`
- `encode_ubfiz(operands)`: unsigned bitfield insert-in-zero alias using the
  same insert mapping
- `encode_bfm(operands)`: raw bitfield move form
- `encode_bfi(operands)`: bitfield insert alias for `BFM`
- `encode_bfxil(operands)`: bitfield extract-and-insert-low alias for `BFM`
- `encode_extr(operands)`: register extract form with `Rd`, `Rn`, `Rm`, and
  immediate lsb
- `encode_clz(operands)`: count leading zero bits
- `encode_cls(operands)`: count leading sign bits
- `encode_rbit(operands)`: scalar bit reverse, with a separate NEON vector
  arrangement form
- `encode_rev(operands)`: scalar byte reverse, selecting a 32-bit or 64-bit
  opcode subfield
- `encode_rev16(operands)`: scalar reverse bytes within halfwords
- `encode_rev32(operands)`: 64-bit scalar reverse bytes within words, with a
  separate NEON vector arrangement form
- `encode_crc32(mnemonic, operands)`: CRC32 and CRC32C variants selected by
  mnemonic spelling and source element size

## Bitfield Encoding Families

The bitfield move family shared the fixed high opcode pattern described in the
comments as:

```text
sf opc 100110 N immr imms Rn Rd
```

The old surface selected `sf` from the destination register width through
`sf_bit(is_64)` and set `N` to `1` for 64-bit registers and `0` for 32-bit
registers. The opcode class bits distinguished the family:

- unsigned forms (`UBFM`, `UBFX`, `UBFIZ`) used `opc = 10`
- signed forms (`SBFM`, `SBFX`, `SBFIZ`) used `opc = 00`
- generic bitfield move forms (`BFM`, `BFI`, `BFXIL`) used `opc = 01`

Alias forms converted `lsb` and `width` into raw `immr` and `imms` values
before writing the word:

- extract aliases used `immr = lsb` and `imms = lsb + width - 1`
- insert-in-zero aliases used `immr = (regsize - lsb) & (regsize - 1)` and
  `imms = width - 1`
- `BFI` used `(reg_width - lsb) % reg_width` for `immr`

The old surface did not show validation for zero width, out-of-range immediates,
or `lsb + width` overflow beyond the selected register width.

## EXTR Encoding

`encode_extr` accepted `Rd`, `Rn`, `Rm`, and an immediate lsb. It recorded the
instruction shape as:

```text
sf 0 0 100111 N 0 Rm imms Rn Rd
```

The implementation placed `Rm` in bits 20:16, the lsb value in bits 15:10, and
used the destination width to set both `sf` and `N`.

## Bit-Manipulation And Reverse Operations

The scalar bit-manipulation routines shared the data-processing one-source
shape with a fixed high pattern equivalent to `sf 1 0 11010110 ... Rn Rd`.
The low opcode subfield selected the specific operation:

- `CLZ` used the subfield recorded as `000100`
- `CLS` used `000101`
- scalar `RBIT` used the base form with no extra low opcode bits
- `REV` selected `000010` for 32-bit registers and `000011` for 64-bit
  registers
- `REV16` selected `000001`
- scalar `REV32` forced the 64-bit form and selected `000010`

`RBIT` and `REV32` also had vector fallbacks when the first operand was a
`RegArrangement`. Those paths depended on NEON helpers and encoded arrangement
bits rather than scalar register width:

- vector `RBIT` treated `16b` as `q = 1`, otherwise `q = 0`
- vector `REV32` used `neon_arr_to_q_size` to derive `q` and `size`

## CRC32 Encoding

`encode_crc32` accepted the mnemonic plus three register operands. It selected
the CRC32C bit when the mnemonic contained `crc32c`, then mapped the mnemonic
suffix to `sf` and `sz`:

- `crc32b` and `crc32cb`: `sf = 0`, `sz = 00`
- `crc32h` and `crc32ch`: `sf = 0`, `sz = 01`
- `crc32w` and `crc32cw`: `sf = 0`, `sz = 10`
- `crc32x` and `crc32cx`: `sf = 1`, `sz = 11`

The default case silently fell back to the byte-sized non-64-bit shape. A
rebuild should reject unknown CRC mnemonics before encoding.

## Dependencies And Assumptions

The removed surface depended on helper contracts from the old encoder module:

- `Operand` from the assembler parser
- `EncodeResult::Word` for single-word output
- `get_reg` for scalar register number and width
- `get_imm` for immediates
- `sf_bit` for scalar width encoding
- `get_neon_reg` and `neon_arr_to_q_size` for vector arrangement forms

It assumed operand counts and operand kinds were already compatible with the
mnemonic. It also used the destination register width as the controlling width
for mixed-width forms and did not cross-check source register widths.

## Failure Risks For Rebuild

- Immediate fields need range checks: `immr`, `imms`, `lsb`, and `width` are
  limited by the selected 32-bit or 64-bit form.
- Alias lowering must reject impossible ranges instead of relying on unsigned
  wrapping arithmetic.
- `BFI`, `BFXIL`, `UBFX`, `SBFX`, `UBFIZ`, and `SBFIZ` should share one tested
  alias-to-raw conversion helper to avoid divergent formulas.
- `EXTR` must validate that all participating registers have compatible widths.
- Vector `RBIT` and `REV32` belong at the boundary between bit-manipulation and
  NEON encoding. A rebuild should decide whether they remain in this surface or
  move to the NEON encoder.
- CRC32 mnemonic dispatch should be table-driven or validated before encoding
  so misspelled mnemonics cannot silently become byte-form CRC32.
- The old commented surface encoded words directly. A rebuilt encoder should
  expose diagnostics that distinguish bad operands, invalid immediate ranges,
  unsupported vector arrangements, and internal encoding mistakes.

## Rebuild Guidance

Use this artifact as the historical map for bitfield-like encoder behavior.
Rebuild the live encoder around structured instruction families, shared
field-packers, and explicit alias validation before reconnecting it to the
assembler or backend output path. For backend-owned output, the input should be
machine instruction nodes or lower structured encoding records, not mnemonic
strings recovered from printed `.s` text.
