# AArch64 Floating-Point Operations Legacy Surface

This artifact preserves the useful production shape from the removed
`float_ops.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/float_ops.rs`.

## Role

The surface implemented AArch64 floating-point binary operation lowering for
`ArmCodegen`. It handled scalar `F32` and `F64` arithmetic with FP/SIMD
instructions while keeping the backend's ordinary temporary/result convention
in integer register `x0`.

Binary128 arithmetic did not lower inline here. `F128` binary operations were
delegated to the shared soft-float binary128 path, and `F128` negation was
delegated to the full-width binary128 negation helper.

## Entry Points

- `emit_float_binop_impl(dest, op, lhs, rhs, ty)`: routes `F128` binary
  operations to `f128_emit_binop`; otherwise materializes both operands through
  `x0`, emits the scalar FP operation body, and stores the resulting raw bits
  back to `dest`.
- `emit_float_binop_body(mnemonic, ty)`: performs the AArch64 register moves
  around the actual `FADD`, `FSUB`, `FMUL`, or `FDIV` instruction for either
  `F32` or `F64`.
- `emit_f128_neg_impl(dest, src)`: delegates full binary128 negation to
  `emit_f128_neg_full`.

## Scalar Binary Operation Flow

The non-`F128` path used `x0` as the shared operand/result bridge even for
floating-point arithmetic:

1. Materialize the left operand into `x0`.
2. Copy that payload to `x1`.
3. Materialize the right operand into `x0`.
4. Copy the right payload from `x0` to `x2`.
5. Move raw integer bits into FP/SIMD registers.
6. Emit the selected scalar FP instruction.
7. Move the FP/SIMD result bits back into `x0`.
8. Store `x0` to the destination.

For `F32`, the bridge was 32-bit at the FP boundary:

```asm
fmov s0, w1
fmov s1, w2
<mnemonic> s0, s0, s1
fmov w0, s0
mov w0, w0
```

The final `mov w0, w0` intentionally zero-extended the single-precision result
payload in the general-purpose register before storage.

For `F64`, the bridge was 64-bit:

```asm
fmov d0, x1
fmov d1, x2
<mnemonic> d0, d0, d1
fmov x0, d0
```

## Binary128 Behavior

`F128` binary operations were explicitly excluded from the scalar FP/SIMD body.
They delegated to the shared binary128 soft-float emitter, which owns operand
materialization, helper calls, result tracking, and storage for operations that
cannot be represented as native AArch64 scalar FP instructions.

`F128` negation likewise stayed out of this scalar path and forwarded to the
full-width binary128 negation helper.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `Operand`, `Value`, and `IrType`.
- Backend operation enum: `FloatOp`.
- AArch64 operand/result helpers: `operand_to_x0` and `store_x0_to`.
- Floating operation mnemonic selection: `emit_float_binop_mnemonic`.
- Binary128 helpers: `f128_emit_binop` and `emit_f128_neg_full`.
- Backend assembly emission through the `state.emit` and `state.emit_fmt`
  paths.
- AArch64 ABI and scratch registers: `x0`, `x1`, `x2`, `w0`, `w1`, `w2`,
  `s0`, `s1`, `d0`, and `d1`.

## Hidden Assumptions

- The ordinary backend temporary convention is that operands are materialized
  into `x0` and stored back out from `x0`, even for scalar floating-point
  values represented as raw bits.
- `x1` and `x2` are available as scratch registers across this lowering body.
- The selected mnemonic already matches the requested operation and scalar
  width; this surface only appends the register-width-specific operands.
- Any non-`F32` scalar type reaching `emit_float_binop_body` is treated as the
  `F64` path.
- Single-precision results require explicit zero-extension after moving `s0`
  back into `w0`.
- Binary128 lowering requires the shared soft-float path rather than native
  scalar AArch64 FP instructions.

## Known Failure Risks For Rebuild

- Rebuilding scalar FP arithmetic as integer arithmetic would operate on raw
  bit patterns instead of floating-point values.
- Dropping the `x1` copy before evaluating the right operand would overwrite
  the left operand in `x0`.
- Omitting the `F32` zero-extension step can leave stale upper bits when the
  single-precision result is later consumed through the general-purpose
  temporary convention.
- Routing `F128` through the scalar `F64` body would truncate semantics and
  bypass required soft-float handling.
- Treating `emit_f128_neg_impl` as a scalar sign-bit flip here could diverge
  from the full binary128 tracking and storage contract.

## Rebuild Guidance

Rebuild this surface around explicit register-file transitions:

Floating-point lowering should model FP/SIMD register transitions in structured
target MIR and machine instruction nodes before any `<mnemonic>` text is
printed. Native FP operations and binary128 helper calls should remain typed
node families rather than parser-recovered assembly strings.

1. Keep the integer-temporary bridge and FP/SIMD arithmetic body visibly
   separate.
2. Preserve distinct `F32`, `F64`, and `F128` routes.
3. Prove that `F32` and `F64` arithmetic move operands into `s`/`d` registers,
   perform the native FP operation, and move raw result bits back to `x0`.
4. Keep `F128` binary operations and negation delegated to the shared
   binary128 helpers unless a later backend design replaces those helpers
   wholesale.
