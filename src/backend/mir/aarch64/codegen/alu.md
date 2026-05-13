# AArch64 ALU Legacy Surface

This artifact preserves the useful production shape from the removed
`alu.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/alu.rs`.

## Role

The surface implemented AArch64 integer ALU lowering plus a small unary
floating-point negation hook for `ArmCodegen`. It covered scalar integer unary
operations, integer bit-counting helpers, byte-swap helpers, binary arithmetic
and bitwise operations, simple register-direct fast paths, power-of-two
unsigned division and remainder reductions, and full accumulator fallback
lowering.

The old code used the backend's ordinary result convention: values were
materialized through `x0`, register-assigned destinations could be updated
directly through callee-saved registers, and fallback results were stored from
`x0`.

## Entry Points

- `emit_float_neg_impl(ty)`: negates an `F32` or `F64` payload by moving raw
  bits between integer and FP/SIMD registers, using `fneg`, and moving the
  result back to the integer temporary convention.
- `emit_int_neg_impl(ty)`: emits integer negation with `neg x0, x0`.
- `emit_int_not_impl(ty)`: emits bitwise complement with `mvn x0, x0`.
- `emit_int_clz_impl(ty)`: emits count-leading-zero for 32-bit or 64-bit
  integer widths.
- `emit_int_ctz_impl(ty)`: emits count-trailing-zero by reversing bits with
  `rbit` and then using `clz`.
- `emit_int_bswap_impl(ty)`: emits byte-swap lowering with `rev`, including a
  16-bit post-shift path.
- `emit_int_popcount_impl(ty)`: uses SIMD byte counts plus horizontal add to
  compute population count.
- `emit_int_binop_impl(dest, op, lhs, rhs, ty)`: lowers integer binary
  operations with power-of-two reductions, register-direct fast paths, and an
  accumulator fallback.
- `emit_copy_i128_impl(dest, src)`: copies a two-register i128 payload through
  `x0:x1`.

## Unary And Bit Operation Behavior

Floating negation treated the input payload as raw bits already in the normal
integer result register. `F32` used `fmov s0, w0`, `fneg s0, s0`, and
`fmov w0, s0`, followed by `mov w0, w0` to zero-extend the 32-bit payload.
Wider floating negation used the `d0` and `x0` form without the extra
zero-extension.

Integer negation and bitwise not ignored the type parameter and operated on
`x0`. Rebuild work that needs exact narrow-width wrapping must decide whether
the caller already normalized `x0` or whether the helper should become
width-aware.

`clz` selected `w0` for `I32` and `U32`, and `x0` otherwise. `ctz` used the
standard AArch64 sequence `rbit` followed by `clz`, again choosing `w0` for
32-bit values and `x0` otherwise.

Byte-swap selected three shapes:

- `I16` and `U16`: `rev w0, w0` followed by `lsr w0, w0, #16`.
- `I32` and `U32`: `rev w0, w0`.
- all other integer widths: `rev x0, x0`.

Population count moved the integer payload into `s0` for 32-bit values or
`d0` for wider values, then used `cnt v0.8b, v0.8b`, `uaddlv h0, v0.8b`, and
`fmov w0, s0`. The result was a 32-bit count in `w0`.

## Binary Operation Reductions

Before choosing the main binary lowering path, unsigned division and remainder
checked whether the right operand was a power-of-two constant through
`const_as_power_of_2(rhs)`.

For `UDiv`, the reduction materialized the left operand into `x0` and emitted
logical shift right by the power-of-two shift amount:

```asm
lsr w0, w0, #<shift>
```

or the `x0` form for non-32-bit values.

For `URem`, the reduction materialized the left operand into `x0`, computed
`mask = (1 << shift) - 1`, and emitted:

```asm
and w0, w0, #<mask>
```

or the `x0` form for non-32-bit values. Both reductions stored the result to
the destination immediately and skipped the general binary lowering path.

## Register-Direct Fast Path

When `dest_reg(dest)` returned a physical destination register, the old
surface tried a register-direct path for simple ALU operations:

- `Add`
- `Sub`
- `And`
- `Or`
- `Xor`
- `Mul`

The mnemonic came from `arm_alu_mnemonic(op)`. For `Add` and `Sub` with an
immediate right operand accepted by `const_as_imm12(rhs)`, the path loaded the
left operand directly into the destination register and emitted an immediate
form. For 32-bit signed results it followed with `sxtw dest_x, dest_w`; for
32-bit unsigned results it left the `w` register write as the zero-extending
operation.

For register or non-imm12 right operands, the path avoided clobbering when the
right operand already occupied the destination register: it loaded the right
operand into `x0` first, then loaded the left operand into the destination
register, and used `x0` or `w0` as the right input. Without that conflict, it
loaded the left operand into the destination register and either referenced the
right operand's existing callee-saved register or materialized the right
operand into `x0`.

After emitting the direct operation, the path invalidated the accumulator
cache and returned without an explicit `store_x0_to`, because the physical
destination register already held the result.

## Accumulator Fallback

All operations not handled by reductions or the register-direct path used the
accumulator route:

1. Materialize the left operand into `x0`.
2. Copy it to `x1`.
3. Materialize the right operand into `x0`.
4. Copy it to `x2`.
5. Emit the selected operation into `x0`.
6. Store `x0` to the destination.

For 32-bit values, the fallback used `w1`, `w2`, and `w0` operations. Signed
32-bit arithmetic and shift results were explicitly sign-extended with
`sxtw x0, w0`; unsigned results relied on the AArch64 architectural
zero-extension of `w0` writes.

The 32-bit fallback covered:

- `add`, `sub`, and `mul`.
- `sdiv` and `udiv`.
- `SRem` as `sdiv w3, w1, w2` followed by `msub w0, w3, w2, w1`.
- `URem` as `udiv w3, w1, w2` followed by `msub w0, w3, w2, w1`.
- `and`, `orr`, and `eor`.
- variable `lsl`, `asr`, and `lsr`.

The 64-bit fallback emitted the same operation family using `x` registers:
`add`, `sub`, `mul`, `sdiv`, `udiv`, `msub`-based signed and unsigned
remainder, `and`, `orr`, `eor`, `lsl`, `asr`, and `lsr`.

## i128 Copy Behavior

The i128 copy helper did not implement arithmetic. It loaded the source
operand into the two-register accumulator pair with `operand_to_x0_x1(src)`
and stored that pair with `store_x0_x1_to(dest)`.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `IrBinOp`, `Operand`, `Value`, and `IrType`.
- Type classification: equality against `I16`, `U16`, `I32`, and `U32`, plus
  `IrType::is_unsigned`.
- Operand and destination helpers: `operand_to_x0`, `operand_to_x0_x1`,
  `operand_to_callee_reg`, `operand_reg`, `dest_reg`, `store_x0_to`, and
  `store_x0_x1_to`.
- Constant classifiers: `const_as_power_of_2` and `const_as_imm12`.
- Register spelling helpers: `callee_saved_name`, `callee_saved_name_32`, and
  `arm_alu_mnemonic`.
- Assembly emission through `state.emit` and `state.emit_fmt`.
- Register-cache behavior through `state.reg_cache.invalidate_acc`.
- AArch64 scratch/result registers: `x0`, `x1`, `x2`, `x3`, their `w`
  aliases, and FP/SIMD registers `s0`, `d0`, and `v0`.

## Hidden Assumptions

- The ordinary operand materialization helpers leave the requested value in
  `x0` and may clobber previous `x0` contents.
- `x1`, `x2`, and `x3` are available scratch registers for the fallback path.
- Register-direct emission is valid only when the destination has an assigned
  physical register.
- Writing a `w` register zero-extends the corresponding `x` register; signed
  32-bit results therefore need explicit `sxtw` when the IR type is signed.
- `const_as_power_of_2(rhs)` is valid for the unsigned division and remainder
  reductions and does not need signedness correction.
- `const_as_imm12(rhs)` captures the immediate shape accepted by AArch64
  `add` and `sub` immediate forms in this old lowering.
- The direct path must handle right-operand destination-register conflicts
  before overwriting the destination with the left operand.
- `emit_int_neg_impl` and `emit_int_not_impl` assume the caller has already
  arranged any required narrow-width normalization.
- Population count's SIMD sequence is acceptable even though the final count is
  moved back through `w0`.

## Known Failure Risks For Rebuild

- Rebuilding signed 32-bit arithmetic without the `sxtw` steps would change
  later 64-bit consumers of signed `I32` results.
- Treating signed division or remainder by powers of two like the unsigned
  reductions would be wrong for negative dividends.
- Omitting the right-operand conflict handling in the register-direct path
  could overwrite an operand before it is used.
- Using the register-direct path for division, remainder, or variable shifts
  without adding equivalent semantics would silently skip operations currently
  handled by the accumulator fallback.
- Forgetting that `Add` and `Sub` immediate fast paths are limited to the
  accepted immediate shape could emit invalid AArch64 assembly.
- Replacing 16-bit byte-swap with plain `rev w0, w0` would leave the swapped
  value in the high half instead of shifting it back down.
- Dropping the `F32` zero-extension after `fneg` could leave stale high bits
  in the integer result convention.
- Treating i128 copy as scalar `x0` storage would lose the high 64 bits.

## Rebuild Guidance

Rebuild this surface around explicit result-width and destination-carrier
decisions:

Integer operation lowering should select typed target MIR operations and then
machine instruction nodes before any final mnemonic spelling is chosen.
`arm_alu_mnemonic`-style names may survive as printer or encoding-table data,
but they must not be the semantic carrier for codegen decisions.

1. Keep unary integer helpers, bit-count helpers, byte-swap helpers, binary
   lowering, and i128 copy as separate routes.
2. Preserve the unsigned power-of-two reductions only for unsigned division and
   remainder unless a signed equivalent is introduced with proof.
3. Keep register-direct ALU emission limited to operations whose result can be
   produced entirely in the assigned destination register.
4. Model 32-bit signed extension and unsigned zero-extension as deliberate
   post-operation behavior, not incidental register spelling.
5. Prove accumulator fallback coverage for division, remainder, variable
   shifts, and all non-register-destination cases.
