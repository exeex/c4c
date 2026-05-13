# AArch64 Encoder Compare Branch Legacy Surface

This artifact preserves the useful structure from the removed
`compare_branch.cpp` translation unit. The old file was a commented C++ mirror
of the reference Rust encoder surface at
`ref/claudes-c-compiler/src/backend/arm/assembler/encoder/compare_branch.rs`.

## Role

The removed surface documented how the legacy assembler encoder handled
AArch64 compare aliases, conditional compare, conditional select aliases, and
direct, conditional, compare-and-branch, and test-and-branch instructions.

It did not provide active compiled C++ behavior in this tree; every routine was
preserved as commented Rust-shaped pseudocode. The surface expected parsed
assembler operands and returned either a single instruction word or an
instruction word paired with relocation metadata.

## Entry Points

The old surface listed these encoder entry points:

- `encode_cmp(operands)`: alias for `SUBS wzr/xzr, Rn, op`
- `encode_cmn(operands)`: alias for `ADDS wzr/xzr, Rn, op`
- `encode_tst(operands)`: alias for `ANDS wzr/xzr, Rn, op`
- `encode_ccmp_ccmn(operands, is_ccmp)`: conditional compare or conditional
  compare-negative, supporting immediate and register second operands
- `encode_csel(operands)`: base conditional select
- `encode_csinc(operands)`: conditional select increment
- `encode_csinv(operands)`: conditional select invert
- `encode_csneg(operands)`: conditional select negate
- `encode_cset(operands)`: alias for `CSINC Rd, xzr/wzr, xzr/wzr, invert(cond)`
- `encode_csetm(operands)`: alias for `CSINV Rd, xzr/wzr, xzr/wzr, invert(cond)`
- `encode_branch(operands)`: unconditional symbolic `B`
- `encode_bl(operands)`: symbolic `BL`
- `encode_cond_branch(cond, operands)`: symbolic `B.cond`
- `encode_br(operands)`: register-indirect `BR`
- `encode_blr(operands)`: register-indirect `BLR`
- `encode_ret(operands)`: `RET`, defaulting to link register `x30`
- `encode_cbz(operands, is_nz)`: symbolic `CBZ` or `CBNZ`
- `encode_tbz(operands, is_nz)`: symbolic `TBZ` or `TBNZ`
- `encode_cneg(operands)`: alias for `CSNEG Rd, Rn, Rn, invert(cond)`
- `encode_cinc(operands)`: alias for `CSINC Rd, Rn, Rn, invert(cond)`
- `encode_cinv(operands)`: alias for `CSINV Rd, Rn, Rn, invert(cond)`

## Compare Aliases

`CMP`, `CMN`, and `TST` were represented as aliases that prepended the zero
register as the destination and then delegated to other encoder families:

- `CMP` delegated to `encode_add_sub(..., is_sub = true, set_flags = true)`
- `CMN` delegated to `encode_add_sub(..., is_sub = false, set_flags = true)`
- `TST` delegated to `encode_logical(..., opc = 0b11)`

The zero register spelling was selected from the first source operand:
`wzr` for 32-bit registers and `xzr` otherwise. The old surface did not show
operand-count validation before alias expansion.

## Conditional Compare

`encode_ccmp_ccmn` selected `sf` from the first register operand and used bit
30 to distinguish the two mnemonics:

- `CCMP` set bit 30
- `CCMN` left bit 30 clear

The immediate form accepted:

```text
Rn, #imm5, #nzcv, cond
```

and placed `imm5` into bits 20:16, the condition into bits 15:12, and the
`nzcv` literal into bits 3:0. The register form accepted:

```text
Rn, Rm, #nzcv, cond
```

and placed `Rm` into bits 20:16. Both forms depended on `encode_cond` and
returned an unsupported-operands error if neither shape matched.

## Conditional Select Family

The base conditional-select encoders shared the same operand shape:

```text
Rd, Rn, Rm, cond
```

`CSEL`, `CSINC`, `CSINV`, and `CSNEG` used the destination register width to
set `sf`, encoded `Rm` in bits 20:16, the condition in bits 15:12, `Rn` in
bits 9:5, and `Rd` in bits 4:0. The variants toggled the opcode bits for
invert, negate, and increment behavior.

Alias encoders used the standard AArch64 inverted-condition forms:

- `CSET Rd, cond` became `CSINC Rd, ZR, ZR, invert(cond)`
- `CSETM Rd, cond` became `CSINV Rd, ZR, ZR, invert(cond)`
- `CNEG Rd, Rn, cond` became `CSNEG Rd, Rn, Rn, invert(cond)`
- `CINC Rd, Rn, cond` became `CSINC Rd, Rn, Rn, invert(cond)`
- `CINV Rd, Rn, cond` became `CSINV Rd, Rn, Rn, invert(cond)`

Condition inversion was recorded as `cond ^ 1`, so a rebuild must reject
invalid condition aliases before applying that transform.

## Branch And Relocation Families

The branch routines split into symbolic relocatable forms and register-direct
forms.

Symbolic branches returned `EncodeResult::WordWithReloc`:

- `B` used base word `0b000101 << 26` and relocation `Jump26`
- `BL` used base word `0b100101 << 26` and relocation `Call26`
- `B.cond` used base word `(0b01010100 << 24) | cond` and relocation
  `CondBr19`
- `CBZ` and `CBNZ` used `sf 011010 op imm19 Rt` and relocation `CondBr19`
- `TBZ` and `TBNZ` used `b5 011011 op b40 imm14 Rt` and relocation `TstBr14`

Register-direct branches returned plain words:

- `BR Rn` used base word `0xd61f0000 | (Rn << 5)`
- `BLR Rn` used base word `0xd63f0000 | (Rn << 5)`
- `RET` used base word `0xd65f0000 | (Rn << 5)` and defaulted to register
  number 30 when no operand was supplied

The symbolic branch helpers depended on `get_symbol` to supply both symbol and
addend. The old surface left all relocation patching to the assembler/linker
path and only emitted the fixed opcode fields plus relocation metadata.

## Dependencies And Assumptions

The removed surface depended on helper contracts from the old assembler and
encoder modules:

- `Operand` from the assembler parser
- `EncodeResult::Word` and `EncodeResult::WordWithReloc`
- `Relocation` and relocation kinds `Jump26`, `Call26`, `CondBr19`, and
  `TstBr14`
- `get_reg` for scalar register number and width
- `parse_reg_num` for explicit `Rm` parsing in conditional compare
- `get_imm` for immediate bit numbers
- `get_symbol` for symbolic branch targets and addends
- `encode_cond` for condition-code names
- `sf_bit` and `is_32bit_reg` for register-width decisions
- `encode_add_sub` and `encode_logical` for compare alias lowering

The surface assumed registers in the same instruction had compatible widths
and did not show range checks for immediate fields, condition aliases beyond
`encode_cond`, or target-range validation before relocation fixup.

## Failure Risks For Rebuild

- Alias lowering for `CMP`, `CMN`, and `TST` must preserve the operand parser's
  immediate, shifted-register, and extended-register forms without inventing
  partial coverage.
- Conditional compare needs explicit range checks for `imm5` and `nzcv`.
- Conditional-select aliases should validate condition names before applying
  `cond ^ 1`; unconditional or always/never encodings need deliberate policy.
- `CSEL`-family encoders should cross-check `Rd`, `Rn`, and `Rm` widths rather
  than only using the destination width.
- `TBZ` and `TBNZ` need bit-number validation against the selected register
  width, including the `b5` split for bits 32 through 63.
- Relocation kinds must remain aligned with the linker range and scaling rules:
  26-bit call/jump targets, 19-bit conditional branch targets, and 14-bit
  test-branch targets are distinct contracts.
- `RET`'s implicit `x30` default should be kept separate from parser-level
  optional-operand handling so diagnostics stay precise.

## Rebuild Guidance

Use this artifact as the historical map for compare and branch encoder
behavior. A rebuilt live encoder should share explicit field-packers for the
conditional-select family, a small alias-lowering layer for compare and
conditional-select aliases, and relocation constructors that make target range
and scaling rules visible at the assembler/linker boundary.
