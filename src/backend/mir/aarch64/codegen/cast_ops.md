# AArch64 Cast Operations Legacy Surface

This artifact preserves the useful production shape from the removed
`cast_ops.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/cast_ops.rs`.

## Role

The surface implemented AArch64 scalar cast lowering for `ArmCodegen`. It
realized the shared `classify_cast(from_ty, to_ty)` decision by emitting the
target instruction sequence that converts the backend's normal accumulator
payload in `x0`/`w0` to the requested destination representation.

The old code used integer register `x0` as the universal temporary and result
carrier, even for floating-point values. Floating-point casts crossed the
integer/FP register-file boundary with `fmov`, performed the conversion in
`s0` or `d0`, then moved the raw result bits back through `x0` or `w0`.

Binary128 casts were not lowered here. The entry-point wrapper first delegated
to the shared F128 soft-float cast path, and only used the default cast route
when that helper declined the operation.

## Entry Points

- `emit_cast_instrs_impl(from_ty, to_ty)`: matches the shared `CastKind` and
  emits the AArch64 instruction body for scalar integer and floating-point
  conversions.
- `emit_cast_impl(dest, src, from_ty, to_ty)`: asks the shared F128 soft-float
  path to handle binary128 casts, then falls back to the shared default cast
  emitter for ordinary source/destination materialization and storage.

## Cast Classification Routes

The lowering trusted `classify_cast` to choose one of these routes:

- `Noop` and `UnsignedToSignedSameSize`: emit no instructions.
- `FloatToSigned`: move an `F32` or `F64` payload into `s0` or `d0`, use
  `fcvtzs x0, <fp>`, then sign-extend for `I8`, `I16`, or `I32` destinations.
- `FloatToUnsigned`: move the floating payload into `s0` or `d0`, use
  `fcvtzu x0, <fp>`, then mask or truncate for `U8`, `U16`, or `U32`
  destinations.
- `SignedToFloat`: sign-extend the source according to its byte width, use
  `scvtf` to produce either `s0` or `d0`, and move the raw FP bits back to the
  integer accumulator.
- `UnsignedToFloat`: use `ucvtf` to produce either `s0` or `d0`, then move the
  raw FP bits back to `x0` or `w0`.
- `FloatToFloat`: use `fcvt d0, s0` for `F32` to `F64` widening, or
  `fcvt s0, d0` for `F64` to `F32` narrowing.
- `SignedToUnsignedSameSize`: apply the destination-width normalization for
  `U8`, `U16`, or `U32`.
- `IntWiden`: zero-extend unsigned `U8`, `U16`, or `U32` sources, and
  sign-extend signed `I8`, `I16`, or `I32` sources.
- `IntNarrow`: normalize the result to the requested integer width and
  signedness.

The F128 `CastKind` variants were treated as unreachable in this instruction
body because the shared classifier was expected not to produce them for this
route; binary128 handling was owned by the outer F128 soft-float delegation.

## Instruction Patterns

Floating-point input values were represented as raw bits in the integer
accumulator before conversion:

```asm
fmov d0, x0
fcvtzs x0, d0
```

or, for single precision:

```asm
fmov s0, w0
fcvtzu x0, s0
```

Integer-to-floating conversions used signed or unsigned conversion
instructions, then returned the result to the integer accumulator convention:

```asm
scvtf d0, x0
fmov x0, d0
```

```asm
ucvtf s0, x0
fmov w0, s0
```

Float widening and narrowing converted between `s0` and `d0`, then moved the
raw result bits back through `x0` or `w0`.

Integer width normalization used these AArch64 forms:

- `sxtb x0, w0` for signed 8-bit results or sources.
- `sxth x0, w0` for signed 16-bit results or sources.
- `sxtw x0, w0` for signed 32-bit results or sources.
- `and x0, x0, #0xff` for unsigned 8-bit results or sources.
- `and x0, x0, #0xffff` for unsigned 16-bit results or sources.
- `mov w0, w0` for unsigned 32-bit truncation/zero-extension.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `Operand`, `Value`, and `IrType`.
- Shared cast classifier: `CastKind` and `classify_cast`.
- Shared cast storage/materialization route:
  `crate::backend::traits::emit_cast_default`.
- Binary128 soft-float route:
  `crate::backend::f128_softfloat::f128_emit_cast`.
- Type queries: `IrType::size()` and `IrType::is_unsigned()`.
- Backend assembly emission through `state.emit`.
- AArch64 accumulator and FP/SIMD registers: `x0`, `w0`, `s0`, and `d0`.

## Hidden Assumptions

- The caller or shared default cast route has already materialized the source
  payload into `x0` or `w0` before `emit_cast_instrs_impl` runs.
- Floating-point values in the accumulator are raw IEEE payload bits, not
  integer numerics; every float cast must cross through `fmov` before `fcvt`.
- Writing `w0` zero-extends the corresponding `x0`; the explicit `mov w0, w0`
  is the old route's unsigned 32-bit normalization.
- Signed narrow and signed 32-bit results require explicit sign-extension so
  later 64-bit consumers see the intended signed value.
- Unsigned 8-bit and 16-bit results require masking because narrower writes do
  not exist through the `x0` accumulator convention.
- `IrType::size()` returns byte widths used for sign-extension of signed
  integer sources before signed-to-float conversion.
- F128 cast variants must be consumed by the soft-float helper before reaching
  the scalar instruction-body match.

## Known Failure Risks For Rebuild

- Treating raw floating-point accumulator bits as integer values would make
  float-to-int and float-to-float casts operate on the wrong representation.
- Dropping the `fmov` bridge around `fcvt` instructions would mix the general
  and FP/SIMD register files incorrectly.
- Rebuilding signed-to-float without pre-extending `I8`, `I16`, and `I32`
  sources would convert stale high bits.
- Omitting signed result extension after float-to-signed or integer narrowing
  would corrupt later users that read the result as a 64-bit signed value.
- Omitting unsigned masks or the `mov w0, w0` truncation would leave high bits
  live after unsigned narrowing.
- Routing F128 casts through this scalar path would bypass the soft-float
  storage, helper-call, and tracking contract.
- Assuming every no-op/same-size cast should normalize bits would change the
  old classifier contract; some routes intentionally emitted nothing.

## Rebuild Guidance

Rebuild this surface around the shared cast classification boundary:

Cast lowering should keep source and destination type facts in structured
target MIR and machine instruction nodes. Concrete move, extend, convert, or
helper-call mnemonics are downstream printer or encoding choices, not parsed
semantic input.

1. Keep source materialization/storage separate from the AArch64 instruction
   body, as the old `emit_cast_impl` did through the default cast route.
2. Preserve distinct signed, unsigned, floating, float-widen/narrow, and
   binary128 routes.
3. Make every integer width-normalization rule explicit in the rebuilt lowering.
4. Keep F128 casts delegated to the binary128 soft-float path unless a later
   design replaces that whole contract.
