# `float_lowering.cpp`

## Purpose And Current Responsibility

This file is the x86 backend's floating-point lowering bucket for unary ops, binary ops, and cast/conversion glue that ultimately emits textual assembly through `X86Codegen::state`. It is not a narrow "float math" leaf: it also owns storage/load helpers for `F128`, integer-width fixups, local label generation, and several register/stack shims needed to bridge the backend's generic value model into x86 SSE or x87 instructions.

The file currently acts as the canonical implementation seam for:

- `F32`/`F64` arithmetic via SSE registers
- `F128` arithmetic and conversion via x87 stack instructions
- integer<->float conversion policy, including unsigned `U64` edge handling
- generic unary-op fallback routing for float and non-float cases
- materialization of `F128` values from constants, slots, allocas, and indirect addresses

The companion header is intentionally minimal; the real API surface still lives as member declarations on the shared `X86Codegen` type.

## Important APIs And Contract Surfaces

The file exposes one free helper entry and a set of `X86Codegen` implementation methods.

```cpp
void emit_unaryop_default(X86Codegen& codegen,
                          const Value& dest,
                          IrUnaryOp op,
                          const Operand& src,
                          IrType ty);
```

Contract: route unary lowering through `operand_to_rax`, dispatch by `IrUnaryOp`, and store the result through `store_rax_to`. Float negation is special-cased; most other operations defer to integer helpers even when called from this file.

```cpp
void X86Codegen::emit_cast_instrs_x86(IrType from_ty, IrType to_ty);
void X86Codegen::emit_generic_cast(IrType from_ty, IrType to_ty);
void X86Codegen::emit_float_binop_impl(const Value& dest,
                                       FloatOp op,
                                       const Operand& lhs,
                                       const Operand& rhs,
                                       IrType ty);
void X86Codegen::emit_unaryop_impl(const Value& dest,
                                   IrUnaryOp op,
                                   const Operand& src,
                                   IrType ty);
```

Contract: these are the public lowering entry points for the x86 backend's float-related operations. They choose between SSE, x87, and integer helper paths based on `IrType`.

```cpp
void X86Codegen::emit_f128_fldt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
void X86Codegen::emit_f128_fstpt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
void X86Codegen::emit_f128_load_to_x87(const Operand& operand);
void X86Codegen::emit_f128_load_finish(const Value& dest);
```

Contract: these methods define how `F128` values enter and leave x87 state. They hide address resolution, stack scratch usage, and the backend bookkeeping that marks some values as living in direct stack slots.

## Dependency Direction And Hidden Inputs

Dependency direction is inward toward broad backend state, not outward from a small numeric abstraction. This file depends on:

- `X86Codegen` register and slot helpers such as `operand_to_rax`, `operand_to_rcx`, `store_rax_to`, `emit_store_result_impl`, `emit_load_ptr_from_slot_impl`, and `emit_alloca_aligned_addr_impl`
- `state.emit(...)` and `state.out.emit_instr_*` string-emission APIs
- `state.get_slot`, `state.is_alloca`, `state.get_f128_constant_words`, `state.reg_cache`, and `state.f128_direct_slots`
- `SlotAddr` shape (`Direct`, `OverAligned`, `Indirect`) plus `value_id`/`ptr_id` side channels
- IR typing policy encoded in `IrType`, including the fact that only `U16`, `U32`, and `U64` are modeled as unsigned helpers here

Hidden inputs and assumptions:

- results are frequently passed through `%rax` even when conceptually floating-point
- `F32`/`F64` bit patterns are marshaled through `%eax`/`%rax` into `%xmm0/%xmm1`
- `F128` is treated as x87-backed storage, with values often reloaded from stack/slot memory rather than tracked in a first-class register abstraction
- several conversions rely on temporary stack slots and exact scratch-stack discipline
- local correctness depends on x86 instruction selection details such as `cvtt*`, `fild*`, `fisttp*`, `fcomip`, `fsubrp`, and `fchs`
- label uniqueness depends on a function-local static counter, not on any backend-global naming service

## Responsibility Buckets

### 1. Type And Width Utilities

Anonymous-namespace helpers classify float/signed/unsigned types, compute storage width, mint local labels, and perform signed narrow-result fixups after 64-bit conversions.

### 2. Unary Lowering

`emit_unaryop_default` and `emit_unaryop_impl` route negation, bitwise not, bit-counting ops, and `F128` negation. The file mixes float-specific and integer-specific unary handling instead of isolating those concerns.

### 3. `F128` Addressing And Storage

`emit_f128_resolve_addr`, `emit_f128_fldt`, `emit_f128_fstpt`, `emit_f128_store_raw_bytes`, `emit_f128_store_f64_via_x87`, `emit_f128_load_to_x87`, and `emit_f128_load_finish` collectively define how extended-precision values are addressed, loaded, stored, and remembered across slots and temporaries.

### 4. Float/Int Cast Routing

`emit_cast_instrs_x86` is the main dispatcher. It splits `F128` conversions away from generic casts, delegates standard SSE conversions to `emit_generic_cast`, and uses dedicated helpers for `U64` and width-extension edge cases.

### 5. Numeric Compatibility Helpers

`emit_int_to_f128_cast`, `emit_f128_to_int_cast`, `emit_f128_to_u64_cast`, `emit_f128_to_f32_cast`, `emit_fild_to_f64_via_stack`, `emit_fisttp_from_f64_via_stack`, `emit_float_to_unsigned`, `emit_int_to_float_conv`, and `emit_u64_to_float` encode ad hoc conversion sequences used to work around instruction-set gaps or signedness traps.

### 6. Binary Float Lowering

`emit_float_binop_impl` chooses x87 for `F128` and SSE scalar ops for `F32`/`F64`, then stores the result back into the backend's generic value representation.

## Notable Fast Paths, Compatibility Paths, And Overfit Pressure

### Core Lowering

- SSE scalar arithmetic for `F32`/`F64` in `emit_float_binop_impl`
- SSE scalar casts between `F32` and `F64`
- standard signed integer<->float conversions using `cvtsi*` and `cvtt*`
- x87 unary negate and x87 arithmetic for `F128`

### Optional Fast Paths

- direct-slot `F128` loads avoid rebuilding an address when `state.f128_direct_slots` already knows the value lives in memory
- constant `F128` operands can be materialized from two stored words without first building a generic operand path
- direct `SlotAddr::Kind::Direct` cases emit `%rbp`-relative forms instead of pointer setup

### Legacy Compatibility

- `U64` conversions use threshold-based split logic with a hard-coded `2^63` double constant and add-back via `INT64_MIN`
- `F128` is repeatedly funneled through stack scratch storage because the backend lacks a cleaner first-class extended-precision abstraction
- `F32 -> F128` and several `F128` conversions are implemented as "via `F64`/stack/x87" compatibility sequences rather than true type-native lowering
- result width repair is done after conversion with explicit sign/zero extension helpers

### Overfit Pressure

- repeated hand-written big-value branches (`u2ld_big`, `ld2u_big`, `f2u_big`, `u2f_big`) show local fixes for specific signedness cliffs rather than one unified conversion model
- constant immediates such as `4890909195324358656LL` encode hidden numeric thresholds without a named contract
- `emit_generic_cast` mixes same-width reinterpret-like cases, width extension, float/integer conversion, and unsigned repair in one dispatcher, which encourages further case-by-case growth
- the file owns both semantic routing and low-level stack choreography, making it easy to patch one failing test by adding another instruction recipe instead of improving backend-wide value representation

## Rebuild Ownership

In a rebuild, this file should own x86-specific floating-point lowering policy and the public lowering seam for float unary/binary/cast operations.

It should not own ad hoc slot-address resolution, generic width-extension utilities, register-cache side effects, or scattered stack-scratch recipes for every conversion corner. Those belong in narrower helpers or lower-level storage/conversion seams that this layer can call instead of re-deriving inline.
