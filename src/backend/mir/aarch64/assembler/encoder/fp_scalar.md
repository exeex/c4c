# AArch64 Encoder FP Scalar Legacy Surface

This artifact preserves the useful structure from the removed
`fp_scalar.cpp` translation unit. The old file was a commented C++ mirror of
the reference Rust encoder surface at
`ref/claudes-c-compiler/src/backend/arm/assembler/encoder/fp_scalar.rs`.

## Role

The removed surface documented how the legacy assembler encoder handled
AArch64 scalar floating-point moves, arithmetic, comparisons, fused
multiply-add/subtract operations, rounding conversions, integer-to-float
conversions, and precision conversions.

It did not provide active compiled C++ behavior in this tree; every routine was
preserved as commented Rust-shaped pseudocode. The surface expected parsed
assembler operands and returned a single encoded instruction word.

## Entry Points

The old surface listed these encoder entry points:

- `encode_fmov(operands)`: register-to-register FMOV across FP registers and
  between GP and FP registers
- `encode_fp_arith(operands, opcode)`: shared FADD, FSUB, FMUL, FDIV-like
  three-register scalar floating-point arithmetic
- `encode_fneg(operands)`: one-source floating-point negate
- `encode_fabs(operands)`: one-source floating-point absolute value
- `encode_fsqrt(operands)`: one-source floating-point square root
- `encode_fp_1src(operands, opcode)`: FRINT-style one-source operations with
  opcode-selected rounding mode
- `encode_fmadd_fmsub(operands, is_sub)`: FMADD/FMSUB four-register fused
  multiply-add/subtract forms
- `encode_fnmadd_fnmsub(operands, is_sub)`: FNMADD/FNMSUB negated fused
  multiply-add/subtract forms
- `encode_fcmp(operands)`: FCMP against another FP register or implicit/zero
  immediate
- `encode_fcvt_rounding(operands, rmode, opcode)`: float-to-integer
  conversion with selected rounding mode and signedness
- `encode_ucvtf(operands)` and `encode_scvtf(operands)`: unsigned and signed
  integer-to-float wrappers
- `encode_int_to_float(operands, is_signed)`: shared SCVTF/UCVTF encoder
- `encode_fcvt_precision(operands)`: FP precision conversion between S, D, and
  H register classes

## FMOV And Register Classes

`encode_fmov` first required two operands and split handling by whether the
destination and source names were floating-point registers:

- FP-to-FP moves used the scalar FP one-source class, with `ftype` selected
  from whether either operand name started with `d`
- GP-to-FP moves selected the `D/X` encoding when the destination started with
  `d`, otherwise the `S/W` encoding
- FP-to-GP moves selected the `X/D` encoding when the source started with `d`,
  otherwise the `W/S` encoding

Immediate FMOV operands were explicitly recorded as unsupported. The old
surface parsed register numbers through `parse_reg_num` and decided FP-ness
through `is_fp_reg`, so it treated spelling as the source of truth for both
register class and scalar width.

## Arithmetic And One-Source Operations

`encode_fp_arith` covered three-register scalar FP arithmetic. The wrapper
opcode selected the operation-specific field while the shared code packed:

```text
0 00 11110 ftype 1 Rm opcode 10 Rn Rd
```

`ftype` was derived only from the destination register name: `d*` meant double
precision and any other accepted spelling fell through to single precision.

The one-source helpers used the same destination-name width rule:

- `FNEG` used the fixed opcode fields for negate
- `FABS` used the fixed opcode fields for absolute value
- `FSQRT` used the fixed opcode fields for square root
- `encode_fp_1src` accepted an `opcode` argument for the FRINTN, FRINTP,
  FRINTM, FRINTZ, FRINTA, FRINTX, and FRINTI-style family

The old surface did not cross-check that the source register class matched the
destination precision.

## Fused Multiply-Add Families

`encode_fmadd_fmsub` and `encode_fnmadd_fnmsub` both consumed four registers:

```text
Rd, Rn, Rm, Ra
```

The regular family packed the scalar FP multiply-add class with the `o1` bit
selected from `is_sub`. The negated family used the same shape and set the
negation bit before packing `Rm`, `Ra`, `Rn`, and `Rd`.

As with the arithmetic helpers, `ftype` came from the destination register
spelling. The commented surface assumed all four registers were compatible FP
registers and left detailed operand diagnostics to helper parsing.

## Compare And Conversion Families

`encode_fcmp` selected source precision from the first operand and handled two
forms:

- `FCMP Fn, #0.0` or omitted second operand used the zero-immediate compare
  encoding
- `FCMP Fn, Fm` used the register compare encoding with `Rm` in bits 20:16

The immediate zero path matched an integer zero operand in the old operand
model; it did not document nonzero floating immediates or exact textual
`#0.0` parsing.

`encode_fcvt_rounding` handled float-to-integer conversion. It selected:

- `sf` from the destination GP register width
- `ftype` from the source FP register spelling
- `rmode` and `opcode` from wrapper arguments

`encode_int_to_float` handled SCVTF and UCVTF in the opposite direction. It
selected:

- `ftype` from the destination FP register spelling
- `sf` from the source GP register width
- `opcode = 010` for signed conversion or `011` for unsigned conversion

`encode_fcvt_precision` handled FP-to-FP precision conversion. It mapped source
and destination register prefixes `s`, `d`, and `h` to the AArch64 `ftype` and
`opc` fields and rejected other spellings.

## Dependencies And Assumptions

The removed surface depended on helper contracts from the old assembler and
encoder modules:

- `Operand` from the assembler parser, especially register and immediate
  operands
- `EncodeResult::Word`
- `get_reg` for register number and GP register width
- `parse_reg_num` for direct register-number extraction in FMOV
- `is_fp_reg` for separating FP and GP register classes

The surface generally inferred scalar precision from register-name prefixes
instead of from typed operands. It assumed operands within each instruction
used compatible register classes and widths unless an explicit helper rejected
them.

## Failure Risks For Rebuild

- FMOV immediate support was missing; a rebuild must decide whether to encode
  AArch64 floating-point modified immediates or reject them with precise
  diagnostics.
- Register-class validation should be explicit. The old code relied on
  spelling checks and did not consistently reject mixed S/D/H widths across
  source and destination operands.
- Precision selection based only on destination spelling can hide malformed
  arithmetic, one-source, and fused operation operands.
- FCMP zero-immediate handling should be tied to the parser's floating literal
  representation, not only an integer zero operand.
- Float-to-integer conversion wrappers need clear mapping for signedness,
  rounding mode, and destination width so FCVTNS/FCVTNU-style variants cannot
  accidentally share the wrong opcode.
- Integer-to-float conversion must validate that GP source width and FP
  destination precision are both intentional, especially for W-to-D and X-to-S
  combinations.
- Half-precision FCVT entries were documented through prefix mapping but the
  surrounding scalar FP helpers mostly distinguished only S and D. A rebuild
  should decide whether H support is complete or isolated to precision
  conversion.

## Rebuild Guidance

Use this artifact as the historical map for scalar floating-point encoder
behavior. A rebuilt live encoder should separate operand classification from
field packing, keep FMOV transfer classes distinct from arithmetic operations,
and centralize scalar FP precision validation before packing instruction
words. For backend-owned output, FP operation identity and operands should
arrive as structured machine instruction nodes or lower encoding records, not
parser operands recovered from printed `.s` text.
