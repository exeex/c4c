# AArch64 NEON Encoder Surface

This file preserves the obsolete
`src/backend/mir/aarch64/assembler/encoder/neon.cpp` surface as markdown for
the markdown-first AArch64 backend reconstruction. The deleted `.cpp` was not
live C++; it was a commented structural mirror of the reference Rust
`backend/arm/assembler/encoder/neon.rs` implementation.

## Source Shape

- Origin: translated from
  `/Users/chi-shengwu/c4c/ref/claudes-c-compiler/src/backend/arm/assembler/encoder/neon.rs`.
- Intended dependencies: assembler `Operand`, `EncodeResult`, `parse_reg_num`,
  `get_reg`, and `get_imm`.
- Primary operand model:
  - `Operand::RegArrangement { reg, arrangement }` for vector registers such as
    `v0.16b`.
  - `Operand::RegLane { reg, elem_size, index }` for lane operands such as
    `v0.h[2]`.
  - `Operand::RegList` and `Operand::RegListIndexed` for structure load/store
    forms.
  - `Operand::Mem` and `Operand::MemPostIndex` for NEON structure memory
    operands.
- Shared helpers:
  - `get_neon_reg` extracts a vector register number plus arrangement string.
  - `neon_arr_to_q_size` maps arrangements to AArch64 `Q` and `size` fields:
    `8b/16b`, `4h/8h`, `2s/4s`, and `1d/2d`.
  - `is_neon_scalar_d_reg_op` detects scalar three-operand `d` register forms.

## Entry Points Preserved

### Generic Vector Arithmetic And Logical Forms

- `encode_cnt`
- `encode_neon_three_same`
- `encode_neon_three_diff`
- `encode_neon_add_sub`
- `encode_neon_mul`
- `encode_neon_pmul`
- `encode_neon_pmull`
- `encode_neon_mla`
- `encode_neon_mls`
- `encode_neon_logical`
- `encode_neon_bic`
- `encode_neon_bsl`
- `encode_neon_bitwise_insert`
- `encode_neon_not`
- `encode_neon_rbit`

These routines encoded the common NEON register layouts around `Q`, `U`,
`size`, `Rm`, opcode bits, `Rn`, and `Rd`. Several wrappers expected opcode
bits from an outer mnemonic dispatch table rather than deriving the mnemonic
inside this file.

### Narrowing, Widening, Saturating, And Shift Forms

- `encode_neon_sqshrun`
- `encode_neon_xtl`
- `encode_neon_two_misc_narrow`
- `encode_neon_shrn`
- `encode_neon_qshrn`
- `encode_neon_three_diff_narrow`
- `encode_neon_shll`
- `encode_neon_shift_imm`
- `encode_neon_ushr`
- `encode_neon_sshr`
- `encode_neon_shl`
- `encode_neon_sli`
- `encode_neon_sri`
- `encode_neon_shift_right`
- `encode_neon_shift_left_imm`

The old code encoded immediate shifts with `immh:immb` formulas based on
element width. It rejected zero or out-of-range shifts for narrowing and
right-shift families. High-half variants were represented by an explicit
`is_high` parameter that usually forced `Q = 1`.

### Compare, Across-Lane, And Miscellaneous Integer Forms

- `encode_neon_cmp_zero`
- `encode_neon_across`
- `encode_neon_addv`
- `encode_neon_across_long`
- `encode_neon_two_misc`

These paths shared the two-register miscellaneous encoding family and relied on
callers to pass signed/unsigned `U` bits and opcode fields. Across-vector
destinations may be scalar registers even when the source operand is a vector
arrangement.

### Element, Lane, And Register-Move Forms

- `encode_neon_elem_long`
- `encode_neon_elem`
- `encode_neon_float_elem`
- `encode_neon_umov`
- `encode_neon_dup`
- `encode_neon_ins`
- `encode_neon_ext`

Lane-indexed encodings split element indices across `H`, `L`, and `M` bits. For
halfword by-element forms the old code limited `Rm` to the low four bits, while
word and double forms used `Rm[4]` as part of the lane selector. `DUP`, `INS`,
and `UMOV` used `imm5` to combine element size and lane index.

### Immediate And Table/Crypto Forms

- `encode_neon_movi`
- `encode_neon_mvni`
- `encode_neon_tbl`
- `encode_neon_tbx`
- `encode_neon_eor3`
- `encode_neon_aes`

`MOVI` and `MVNI` expanded immediate bytes into the `abc:defgh`, `cmode`, and
operation-bit fields, with special handling for `.2d` byte-mask immediates and
optional `LSL`/`MSL` shifts. `TBL` and `TBX` derived the `len` field from the
register-list length. AES and SHA3-adjacent helpers were direct opcode
encoders.

### NEON Structure Loads And Stores

- `encode_neon_ld1r`
- `encode_neon_ldnr`
- `encode_neon_ld_st_dispatch`
- `encode_neon_ld_st_single`
- `encode_neon_ld_st_multi`

The dispatch selected single-structure element encodings when the first operand
was `RegListIndexed`, otherwise multiple-structure encodings. Structure count,
register count, element size, index, post-index form, and load/store direction
fed `opcode`, `S`, `Q`, `size`, `R`, and `L` fields.

Important old assumptions:

- Consecutive register-list validation was marked TODO and was not implemented.
- Single-structure post-index support existed, but comments still noted TODO
  coverage for some post-index forms.
- Immediate post-index used `Rm = 11111`; register post-index encoded the
  parsed `Rm`.
- Some offsets were parsed but not validated against the implicit element or
  structure byte count.

### Floating-Point Vector And Scalar Forms

- `encode_neon_float_three_same`
- `encode_neon_float_two_misc`
- `encode_neon_float_cmp_zero`
- `encode_neon_fcvtl`
- `encode_neon_fcvtn`
- `encode_neon_faddp`
- `encode_neon_scalar_three_same`
- `encode_neon_scalar_addp`
- `encode_neon_scalar_two_misc`
- `encode_neon_scalar_qshrn`

Float vector paths were limited to `2s`, `4s`, and `2d` arrangements, with
`size_hi` and `sz` forming the encoded size. `FADDP` had both vector and scalar
forms. Scalar NEON paths parsed plain scalar register names and derived size
from register prefixes such as `b`, `h`, `s`, and `d`.

## Hidden Assumptions And Rebuild Risks

- The file assumed an outer mnemonic dispatch supplied correct opcode fields,
  signedness bits, high-half flags, and rounding flags.
- Arrangement matching was string-based and lower-case-sensitive in most paths.
- Many helpers accepted only the subset of arrangements observed by the old
  tests; unsupported arrangements returned an error rather than falling back.
- Several encoders masked lane indices instead of fully validating architectural
  bounds, especially `DUP`, `INS`, and `UMOV`.
- Some formulas used compact bit shifts that should be rechecked against the
  Arm ARM before reimplementation, especially immediate shifts, structure
  load/store post-indexing, `MOVI/MVNI`, and scalar narrowing.
- The old surface did not prove live AArch64 backend behavior. Treat this file
  as reconstruction guidance for operand contracts, dispatch boundaries, and
  encoding families only.

## Reconstruction Notes

- Rebuild this area around a typed SIMD operand model rather than passing
  arrangement strings and ad hoc opcode bits through many helper calls.
- Keep structure load/store encoding separate from scalar/vector arithmetic;
  it has different operand validation and post-index rules.
- Keep lane-index encoding helpers shared across by-element, `DUP`, `INS`, and
  `UMOV`, but make bounds validation explicit per element width.
- Revalidate every immediate encoding against authoritative AArch64 encoding
  tables before making it live.
