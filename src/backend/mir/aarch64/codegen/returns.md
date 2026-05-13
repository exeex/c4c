# AArch64 Returns Legacy Surface

This artifact preserves the useful production shape from the removed
`returns.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/returns.rs`.

## Role

The surface implemented `ArmCodegen` return lowering hooks. It connected
default return emission, binary128 return handling, scalar return-register
materialization, multi-register floating-point returns, and the current
function return type tracked by surrounding prologue state.

Most scalar returns delegated to shared backend behavior or were explicit
no-ops because the value was already in the ABI return register. The main
target-specific responsibilities were moving floating-point bit patterns from
integer temporaries into AArch64 FP/SIMD return registers and preserving the
second return component for aggregate-like floating-point results.

## Entry Points

- `emit_return_impl(val, frame_size)`: checks the current function return type;
  `long double`/`F128` returns are materialized into `q0` before emitting the
  epilogue and `ret`, while other returns delegate to the shared default return
  path.
- `emit_return_i128_to_regs_impl()`: leaves `x0:x1` untouched because the
  `i128` return value is already in the AAPCS64 return registers.
- `emit_return_f128_to_reg_impl()`: treats `x0` as a double-width source,
  moves it into `d0`, then calls `__extenddftf2` to produce the binary128
  return in the expected FP/SIMD location.
- `emit_return_f32_to_reg_impl()`: moves the low 32 bits from `w0` into `s0`.
- `emit_return_f64_to_reg_impl()`: moves the 64-bit payload from `x0` into
  `d0`.
- `emit_return_int_to_reg_impl()`: leaves `x0` untouched because integer
  return values are already in the ABI return register.
- `current_return_type_impl()`: returns the tracked current function return
  type.
- `emit_get_return_f64_second_impl(dest)`: stores `d1` into the destination
  stack slot.
- `emit_set_return_f64_second_impl(src)`: loads or materializes the source and
  moves the second `f64` return component into `d1`.
- `emit_get_return_f32_second_impl(dest)`: stores `s1` into the destination
  stack slot.
- `emit_set_return_f32_second_impl(src)`: loads or materializes the source and
  moves the second `f32` return component into `s1`.
- `emit_get_return_f128_second_impl(dest)`: stores `q1` into the destination
  stack slot and marks the destination as tracking an `F128` value.
- `emit_set_return_f128_second_impl(src)`: materializes the source in `q0` and
  copies the full vector payload into `q1`.

## Return Register Behavior

Integer and `i128` returns assumed prior lowering had already placed the return
payload in AAPCS64 registers: `x0` for scalar integer returns and `x0:x1` for
`i128`. Those hooks intentionally emitted no instructions.

Floating-point scalar return hooks treated `x0`/`w0` as the canonical temporary
register that held the raw bit pattern before the final ABI move:

```asm
fmov s0, w0
fmov d0, x0
```

The binary128 return path was special. Direct `F128` function returns loaded
the operand into `q0` before running the common epilogue. The narrower
conversion hook moved `x0` into `d0` and called `__extenddftf2`, so rebuild
work should preserve the distinction between already-binary128 values and
values that must be widened through the soft-float helper.

## Second Return Component Behavior

The surface exposed get/set helpers for the second floating-point return
register. These were stack-slot based for reads from the return registers and
operand-materialization based for writes into the return registers.

For `F64`, value operands were loaded from their stack slot into `d1`;
constants were loaded as raw 64-bit bits into `x0` and moved into `d1`; all
other operands went through `operand_to_x0` and then `fmov d1, x0`.

For `F32`, value operands were loaded from their stack slot into `s1`;
constants were loaded as raw 32-bit bits through `emit_load_imm64` and moved
with `fmov s1, w0`; all other operands went through `operand_to_x0` and then
`fmov s1, w0`.

For `F128`, reads stored the full `q1` register and recorded destination
tracking through `track_f128_self`. Writes materialized the source into `q0`
with the full binary128 operand helper, then copied all 128 bits with
`mov v1.16b, v0.16b`.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `IrConst`, `Operand`, `Value`, and `IrType`.
- Shared backend return behavior: `emit_return_default`.
- AArch64 frame behavior: `emit_epilogue_and_ret_impl`.
- Operand and result helpers: `operand_to_x0`, `emit_f128_operand_to_q0_full`,
  `emit_load_imm64`, `emit_store_to_sp`, and `emit_load_from_sp`.
- Backend state: current return type, stack-slot lookup, assembly emission,
  and `F128` self-tracking.
- AArch64 ABI registers: `x0`, `x1`, `w0`, `s0`, `s1`, `d0`, `d1`, `q0`, and
  `q1`.
- Soft-float runtime helper: `__extenddftf2`.

## Hidden Assumptions

- `emit_return_impl` relies on `current_return_type` being established before
  return lowering, typically by prologue or function-entry setup.
- Shared default return emission owns the ordinary epilogue path for non-F128
  returns.
- Integer temporaries carry floating-point return payloads as raw bits until
  the final `fmov` into FP/SIMD ABI registers.
- Stack slots for second return components already exist when get helpers are
  called; missing slots silently produce no emission.
- The second return register helpers assume `d1`, `s1`, and `q1` are the ABI
  locations expected by the higher-level aggregate or multi-result lowering
  path.
- `F128` second-component storage must update backend tracking state, unlike
  the `F32` and `F64` paths.

## Known Failure Risks For Rebuild

- Treating all floating-point returns as ordinary integer returns would leave
  raw bits in `x0` instead of the ABI-required FP/SIMD registers.
- Reusing the `F64` path for `F32` would move from `x0`/`d1` instead of
  `w0`/`s1`, changing the register width and potentially stale upper bits.
- Rebuilding `F128` return lowering without the direct `q0` path and the
  `__extenddftf2` widening path can conflate two distinct contracts.
- Dropping `track_f128_self` after storing `q1` can make later binary128
  lowering lose ownership information for the stored value.
- Assuming missing stack slots are impossible would change the archived
  behavior, which simply skipped emission when lookup failed.

## Rebuild Guidance

Rebuild return lowering around ABI register ownership:

Return lowering should publish structured target MIR return facts and machine
instruction nodes for value movement and control transfer. Final `ret`,
register-spelling, and helper-call text belongs to printer or encoding/object
consumers.

1. Keep scalar integer, `i128`, scalar floating-point, and binary128 return
   register rules separate.
2. Preserve the distinction between raw-bit moves into FP/SIMD registers and
   soft-float helper calls.
3. Keep second return component helpers explicit for `F32`, `F64`, and `F128`
   widths instead of routing all cases through one generic integer path.
4. Ensure function-entry state initializes the current return type before any
   return emission consults it.
5. Add proof that ordinary returns, `F128` returns, and second FP return
   components all land in the expected AAPCS64 registers.
