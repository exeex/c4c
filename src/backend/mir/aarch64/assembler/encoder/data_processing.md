# AArch64 Encoder Data Processing Legacy Surface

This artifact preserves the useful structure from the removed
`data_processing.cpp` translation unit. The old file was a commented C++ mirror
of the reference Rust encoder surface at
`ref/claudes-c-compiler/src/backend/arm/assembler/encoder/data_processing.rs`.

## Role

The removed surface documented how the legacy assembler encoder handled
AArch64 scalar data-processing instructions, selected aliases, some NEON
fallbacks, and relocation-bearing immediate forms.

It did not provide active compiled C++ behavior in this tree; every routine was
preserved as commented Rust-shaped pseudocode. The surface expected operands
from the legacy assembler parser and returned either one instruction word, a
small sequence of words, or a word paired with relocation metadata.

## Entry Points

The old surface listed these encoder entry points:

- `encode_mov(operands)`: MOV aliases for scalar immediates, scalar register
  moves, SP moves, and NEON register, lane-insert, lane-extract, and
  element-to-element moves
- `encode_mov_wide_imm(rd, is_64, imm)`: MOVZ plus MOVK sequence generation
  for large immediates
- `resolve_abs_g_modifier(kind, symbol)`: constant folding for `:abs_g*:`
  modifier chunks used by MOVZ/MOVK
- `encode_movz(operands)`, `encode_movk(operands)`, and
  `encode_movn(operands)`: move-wide immediate encoders with optional
  `lsl #N` halfword selectors
- `encode_add_sub(operands, is_sub, set_flags)`: ADD, ADDS, SUB, and SUBS
  immediate, shifted-register, extended-register, SP, NEON, and relocatable
  low-12/TLS forms
- `encode_logical(operands, opc)`: AND, ORR, EOR, and flag-setting logical
  forms with bitmask immediates, shifted registers, and NEON delegation
- `encode_bitmask_imm(val, is_64)`: AArch64 logical-immediate recognizer
- `encode_mul(operands)`, `encode_madd(operands)`,
  `encode_msub(operands)`, and `encode_div(operands, unsigned)`:
  multiply, multiply-add/subtract, and divide forms
- `encode_smull(operands)`, `encode_umull(operands)`,
  `encode_smaddl(operands)`, `encode_umaddl(operands)`,
  `encode_mneg(operands)`, `encode_umulh(operands)`, and
  `encode_smulh(operands)`: long multiply and high-half multiply aliases or
  base forms
- `encode_neg(operands)` and `encode_negs(operands)`: aliases for subtracting
  from the zero register, with optional shifted source operands
- `encode_mvn(operands)`: scalar ORN-from-zero alias plus NEON NOT delegation
- `encode_adc(operands, set_flags)` and `encode_sbc(operands, set_flags)`:
  add/subtract-with-carry families
- `encode_shift(operands, shift_type)`: LSL, LSR, ASR, and ROR immediate and
  register forms
- `encode_sxtw(operands)`, `encode_sxth(operands)`,
  `encode_sxtb(operands)`, `encode_uxtw(operands)`,
  `encode_uxth(operands)`, and `encode_uxtb(operands)`: sign/zero extension
  aliases
- `encode_orn(operands)`, `encode_eon(operands)`,
  `encode_bics(operands)`, and `encode_bic(operands)`: logical NOT and
  bit-clear forms, including selected NEON vector dispatch

## Move And Move-Wide Forms

`encode_mov` was an alias dispatcher with several unrelated operand shapes:

- NEON register-to-register moves lowered to `ORR Vd.T, Vm.T, Vm.T`
- NEON scalar lane insertion lowered to `INS Vd.T[index], Rn`
- NEON lane extraction lowered to `UMOV Rd, Vn.T[index]`
- NEON lane-to-lane moves lowered to `INS Vd.T[index], Vn.T[index]`
- scalar `mov Rd, #imm` tried MOVZ, MOVN for negative small immediates,
  ORR-with-bitmask-immediate, and finally MOVZ/MOVK sequencing
- scalar `mov Rd, Rm` lowered to ORR from XZR/WZR, except SP moves used
  ADD with immediate zero to preserve SP semantics

The move-wide family used the standard AArch64 halfword selector field. MOVZ
used the `10100101` high pattern, MOVK used `11100101`, and MOVN used
`00100101`. Optional `lsl #N` operands were converted to `hw = N / 16`.

`resolve_abs_g_modifier` recognized `abs_g0`, `abs_g0_nc`, `abs_g0_s`,
`abs_g1`, `abs_g1_nc`, `abs_g1_s`, `abs_g2`, `abs_g2_nc`, `abs_g2_s`, and
`abs_g3`. It folded pure integer expressions locally and returned `None` when
the modifier referenced a symbol requiring relocation. The commented surface
did not construct relocation records for symbolic `abs_g*` MOVZ/MOVK forms.

## Add, Subtract, And Relocatable Immediates

`encode_add_sub` shared one implementation for ADD/SUB and flag-setting
variants. It selected `op` from `is_sub`, `S` from `set_flags`, and `sf` from
the destination register width.

Immediate forms supported:

- negative immediates by swapping ADD and SUB
- unshifted 12-bit immediates
- explicit `lsl #12`
- automatic `lsl #12` when the low 12 bits were zero

Relocatable modifier forms produced `EncodeResult::WordWithReloc`:

- `:lo12:symbol` and `:lo12:symbol+offset` used `RelocType::AddAbsLo12`
- `:tprel_lo12_nc:symbol` used `RelocType::TlsLeAddTprelLo12`
- `:tprel_hi12:symbol` used `RelocType::TlsLeAddTprelHi12` and set the
  shifted-immediate bit

Register forms handled explicit extend operands (`uxtb`, `uxth`, `uxtw`,
`uxtx`, `sxtb`, `sxth`, `sxtw`, and `sxtx`), implicit SP-preserving extended
forms when `Rd` or `Rn` was SP, and shifted-register forms with `lsl`, `lsr`,
or `asr`.

## Logical And Bitmask Immediate Forms

`encode_logical` handled scalar AND/ORR/EOR-like operations through an `opc`
parameter and delegated vector arrangements to the NEON logical helper. Scalar
immediate forms depended on `encode_bitmask_imm`; register forms accepted an
optional `lsl`, `lsr`, `asr`, or `ror` shift.

`encode_bitmask_imm` rejected all-zero and all-one masks, tried element sizes
2, 4, 8, 16, 32, and 64, checked that the element pattern repeated across the
register width, then searched for a rotated contiguous run of one bits. It
returned the AArch64 `(N, immr, imms)` fields for logical-immediate encoding.

The old logical-NOT family shared the same shifted-register shape with the N
bit set:

- `ORN` used ORR-with-N and also had a NEON vector form
- `EON` used EOR-with-N
- `BICS` used ANDS-with-N
- `BIC` dispatched between NEON vector form, inverted bitmask immediate form,
  and scalar shifted-register form

## Multiply, Divide, Carry, And Alias Families

The multiply/divide portion covered the data-processing two-source and
three-source instruction classes:

- `MUL` lowered to `MADD Rd, Rn, Rm, XZR`
- `MADD` and `MSUB` encoded the explicit accumulator register
- `SDIV` and `UDIV` shared `encode_div` with the signedness bit
- `SMULL` and `UMULL` lowered to `SMADDL` and `UMADDL` with `Ra = XZR`
- `SMADDL` and `UMADDL` encoded the explicit addend register
- `MNEG` lowered to `MSUB Rd, Rn, Rm, XZR`
- `UMULH` and `SMULH` used fixed 64-bit high-half multiply patterns

`ADC`, `ADCS`, `SBC`, and `SBCS` shared the add/subtract-with-carry encoding
shape, with the subtract and flag-setting bits selected by wrapper arguments.

`NEG`, `NEGS`, and scalar `MVN` were represented as aliases over SUB/SUBS from
the zero register and ORN from the zero register. These aliases accepted the
same shifted-register suffixes as the underlying logical or arithmetic forms.

## Shifts And Extension Aliases

`encode_shift` handled immediate and register shift forms for LSL, LSR, ASR,
and ROR:

- immediate LSL lowered to `UBFM Rd, Rn, #(-imm mod width), #(width - 1 - imm)`
- immediate LSR lowered to `UBFM Rd, Rn, #imm, #(width - 1)`
- immediate ASR lowered to `SBFM Rd, Rn, #imm, #(width - 1)`
- immediate ROR lowered to `EXTR Rd, Rn, Rn, #imm`
- register shifts used the data-processing two-source shift opcode field

The extension aliases were encoded through bitfield or MOV-like forms:

- `SXTW` used 64-bit `SBFM` with `imms = 31`
- `SXTH` and `SXTB` used `SBFM` with `imms = 15` or `7`
- `UXTW` used a 32-bit ORR/MOV alias
- `UXTH` and `UXTB` used `UBFM` with `imms = 15` or `7`

## Dependencies And Assumptions

The removed surface depended on helper contracts from the old assembler and
encoder modules:

- `Operand` from the assembler parser, including register, immediate, shift,
  extend, modifier, modifier-offset, register-arrangement, and register-lane
  forms
- `EncodeResult::Word`, `EncodeResult::Words`, and
  `EncodeResult::WordWithReloc`
- `Relocation` and relocation kinds `AddAbsLo12`, `TlsLeAddTprelLo12`, and
  `TlsLeAddTprelHi12`
- `get_reg`, `parse_reg_num`, `is_64bit_reg`, `sf_bit`, and `get_imm`
- `encode_bitmask_imm` for MOV-as-ORR and logical immediate forms
- `parse_integer_expr` for constant-only `abs_g*` modifier folding
- NEON helpers including `get_neon_reg`, `encode_neon_add_sub`,
  `encode_neon_logical`, `encode_neon_mul`, `encode_neon_not`, and
  `encode_neon_bic`

The surface generally used the destination register width as the controlling
width and did not consistently cross-check source widths, lane arrangements,
immediate ranges, or shift ranges. Several paths defaulted unknown shift or
extend names to a valid encoding rather than rejecting the operand.

## Failure Risks For Rebuild

- MOV alias dispatch combines scalar, SP, immediate, and NEON lane behavior;
  a rebuild should split those forms before encoding so diagnostics stay
  precise.
- MOVZ/MOVK symbolic `abs_g*` handling needs a deliberate relocation policy.
  The old constant-only helper returned `None` for symbols rather than
  emitting relocation metadata.
- Add/sub immediate handling should range-check explicit `lsl #12` operands
  instead of masking to 12 bits.
- Register add/sub must preserve SP semantics by using extended-register forms
  where shifted-register encoding would mean XZR/WZR.
- Logical bitmask immediate encoding should be tested against architectural
  edge cases, especially rotations, 32-bit truncation, and all-ones/all-zero
  rejection.
- Alias lowering for shifts and extensions needs explicit immediate-range
  checks to avoid unsigned wrapping in `width - imm` and related formulas.
- NEON fallbacks in MOV, ADD/SUB, logical, MUL, MVN, ORN, and BIC cross into
  the vector encoder surface. A rebuild should decide whether these stay in
  this module or move behind a shared vector dispatch layer.
- Carry, multiply-long, and high-half multiply forms should validate operand
  widths, especially W-source/X-destination expectations for long multiply
  aliases.
- Unknown shift or extend names should be rejected before encoding instead of
  silently becoming LSL or UXTX-like defaults.
- Relocation constructors for low-12 and TLS modifiers must stay aligned with
  linker fixup scaling, overflow, and addend behavior.

## Rebuild Guidance

Use this artifact as the historical map for scalar data-processing encoder
behavior. A rebuilt live encoder should separate alias lowering from field
packing, centralize width and immediate validation, share logical-immediate
recognition with bitfield/alias users, and keep relocation creation explicit at
the assembler/linker boundary.
