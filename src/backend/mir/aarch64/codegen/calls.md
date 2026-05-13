# AArch64 Calls Legacy Surface

This artifact preserves the useful production shape from the removed
`calls.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/calls.rs`.

## Role

The surface implemented AArch64 function-call lowering for `ArmCodegen`.
It encoded target ABI policy, stack argument staging, register argument
loading, direct and indirect branch emission, post-call stack cleanup, and
wide-result storage.

The old lowering used `x0` as the primary accumulator and first integer
argument/result register, `x1` as the secondary integer argument/result
register, `q0` as the primary F128/FP result carrier, `q1` as temporary F128
result state that must survive a truncation helper call, `x16` and `x17` as
large-immediate and indirect-call scratch registers, and `x9`/`x10` as volatile
scratch registers that were preserved around the F128 extension helper when
needed.

## Entry Points

- `call_abi_config_impl()`: returned the AArch64 call ABI configuration used by
  shared call-classification code.
- `emit_call_compute_stack_space_impl(arg_classes, arg_types)`: delegated stack
  space computation to `compute_stack_arg_space`.
- `emit_call_f128_pre_convert_impl(args, arg_classes, arg_types,
  stack_arg_space)`: reserved no extra F128 conversion area and returned zero.
- `emit_call_stack_args_impl(args, arg_classes, arg_types, stack_arg_space,
  fptr_spill, f128_temp_space)`: staged stack-passed arguments below `sp`.
- `emit_call_reg_args_impl(args, arg_classes, arg_types, total_sp_adjust,
  f128_temp_space, stack_arg_space, struct_arg_riscv_float_classes)`: loaded
  register-passed arguments through helper phases.
- `emit_call_instruction_impl(direct_name, func_ptr, indirect,
  stack_arg_space)`: emitted the direct `bl` or indirect `blr` call.
- `emit_call_cleanup_impl(stack_arg_space, f128_temp_space, indirect)`:
  restored stack space used for call arguments and indirect function-pointer
  spill storage.
- `emit_call_store_i128_result_impl(dest)`: stored the `x0`/`x1` result pair.
- `emit_call_store_f128_result_impl(dest)`: stored the F128 `q0` result and
  also materialized the compatible `double` view through `__trunctfdf2`.

## ABI Configuration

The legacy AArch64 call configuration carried these policy facts:

- Up to eight integer argument registers and eight floating-point argument
  registers.
- I128 register pairs are aligned.
- F128 values are passed in FP registers, not GP register pairs.
- Variadic floating-point arguments are not duplicated into GP registers by
  this surface.
- Large structs are passed by reference.
- SysV and RISC-V float struct classification are disabled.
- Structs cannot be split between registers and the stack.
- Struct register pairs are not specially aligned.
- Structure return uses a dedicated register.

Stack-space sizing was intentionally shared with the common call ABI helper by
calling `compute_stack_arg_space(arg_classes)`.

## Stack Argument Lowering

When stack arguments were present, the old code first subtracted
`stack_arg_space` from `sp`. If the function had no dynamic alloca, it adjusted
source stack-slot loads by `stack_arg_space + fptr_spill`; with dynamic alloca
it used source slots without that additional offset.

Stack placement walked arguments in order and ignored non-stack classes.
`I128Stack` and `F128Stack` arguments were aligned to 16 bytes by rounding the
running stack offset up before storing.

By-value and large-struct stack arguments treated the operand as an address and
copied `ceil(size / 8)` doublewords to the outgoing stack area. The source
address came from an assigned callee-saved register, an alloca address, a
loaded stack slot, a constant operand converted through the accumulator path,
or a zero fallback. Each doubleword was loaded through `x0` plus an offset and
stored to raw `sp`.

I128 stack arguments stored two eight-byte lanes. I128 constants split the low
and high halves into separate immediate loads. Non-I128 constants fell back to
the accumulator for the low lane and zeroed the high lane. Value operands loaded
both lanes from an ordinary slot when available; alloca operands stored the
alloca address as the low lane and zeroed the high lane. Missing operands
stored zeros for both lanes.

F128 stack arguments stored sixteen bytes. Constants used exact
`LongDouble` bytes when available, otherwise converted the constant through
`f64_to_f128_bytes`. Value operands first tried recorded F128 source metadata:
direct sources loaded a `q0` value from a source stack slot plus offset, while
indirect sources loaded the pointer into `x17`, applied the byte offset, then
loaded `q0` from `[x17]`. If no full F128 source could be recovered, the value
was materialized as a double in `d0`, `x9` and `x10` were saved with
`stp`, `__extenddftf2` was called, `x9` and `x10` were restored with `ldp`,
and `q0` was stored to the outgoing stack slot.

Ordinary stack arguments used an assigned callee-saved register, alloca
address, stack-slot load, constant conversion, or zero fallback to populate
`x0`, then stored `x0` to the outgoing stack slot and advanced by eight bytes.

The stack-argument helper returned the total call-time stack adjustment:
`stack_arg_space + fptr_spill`.

## Register Argument Lowering

Register argument lowering was factored into helper phases rather than encoded
directly in this surface. The slot adjustment was `total_sp_adjust` unless
dynamic alloca was active, in which case it stayed zero. The old code also
flagged whether adjusted loads were needed whenever `total_sp_adjust > 0`.

The helper order was significant:

1. Move GP register arguments into temporary locations.
2. Load FP register arguments.
3. Move temporary GP values into final ABI argument registers.
4. Load I128 register-pair arguments.
5. Load by-value struct register arguments.

This sequencing preserved source values while outgoing stack and register
arguments were being rearranged.

## Call Emission And Cleanup

Direct calls emitted:

```asm
bl <name>
```

Indirect calls expected the function pointer to be spilled above the outgoing
stack argument area. The spill offset was `stack_arg_space`. If that offset fit
the AArch64 immediate load form, the old path emitted:

```asm
ldr x17, [sp, #<stack_arg_space>]
blr x17
```

For larger offsets it loaded the offset into `x17`, added it to `sp`, loaded
the function pointer from `[x17]`, then emitted `blr x17`.

Cleanup added back `stack_arg_space` plus a 16-byte indirect-call spill area
when the call was indirect. No additional F128 temporary area was accounted for
by this surface.

## Result Handling

I128 results were stored by writing the `x0` and `x1` result registers to the
destination with the shared `store_x0_x1_to` helper.

F128 results first stored `q0` to the destination stack slot when a destination
slot existed, then tracked that destination as self-sourced F128 state. The old
path then reserved 16 bytes on the stack, saved `q1`, called `__trunctfdf2`,
moved the converted `d0` bits into `x0`, restored `q1`, released the temporary
stack space, invalidated the register cache, and marked the accumulator cache
entry for the destination as not integer-current.

The F128 result path therefore preserved both the full quad-precision memory
result and a compatible accumulator-style double view for surrounding lowering
that still expected `x0` to describe the returned value.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `Operand`, `Value`, `IrConst`, and `IrType`.
- Shared call ABI model: `CallAbiConfig`, `CallArgClass`, and
  `compute_stack_arg_space`.
- Backend state facts: stack slots, alloca markers, dynamic alloca state,
  assigned callee-saved registers, F128 source tracking, and register-cache
  invalidation.
- AArch64 helpers: `emit_sub_sp`, `emit_add_sp`, `emit_alloca_addr`,
  `emit_load_from_sp`, `emit_load_from_reg`, `emit_store_to_raw_sp`,
  `emit_store_to_sp`, `emit_load_imm64`, `load_large_imm`,
  `operand_to_x0`, `store_x0_x1_to`, and `callee_saved_name`.
- Softfloat/compiler-rt helpers: `__extenddftf2` for double-to-F128 argument
  conversion and `__trunctfdf2` for F128 result truncation.
- AArch64 call instructions `bl` and `blr`, raw SP-relative stack stores, and
  GP/FP ABI result registers.

## Hidden Assumptions

- The shared classifier has already assigned each argument to a compatible
  `CallArgClass`; this surface only realizes that decision.
- Outgoing stack slots are written at raw offsets from the adjusted `sp`.
- Source stack slots need `stack_arg_space + fptr_spill` correction only when
  dynamic alloca is not active.
- Indirect function pointers are spilled at the outgoing stack area offset and
  require a 16-byte spill reservation.
- F128 stack values are always 16-byte aligned.
- `x9` and `x10` may contain live scratch state across `__extenddftf2`, so the
  fallback F128 conversion path saves and restores them explicitly.
- F128 source metadata distinguishes direct stack-backed quad values from
  indirect pointer-backed quad values.
- Missing stack slots or unresolved operands use zero fallbacks instead of
  forcing a diagnostic in this legacy route.
- The result destination for F128 may need both stack-slot `q0` storage and
  accumulator-cache updates after truncation.

## Known Failure Risks For Rebuild

- Dropping the AArch64 ABI configuration would make shared classification drift
  toward another target's register counts, F128 policy, or struct-passing
  behavior.
- Failing to adjust source stack-slot loads after subtracting outgoing call
  space would read from the wrong frame location for non-dynamic-alloca calls.
- Treating by-value structs as scalar values would pass only a pointer-sized
  fragment instead of copying the outgoing object bytes.
- Losing 16-byte alignment for I128 and F128 stack arguments would violate the
  intended stack layout.
- Recomputing F128 values only through double conversion would lose exact
  quad-precision values when F128 source metadata is available.
- Omitting the `x9`/`x10` save around `__extenddftf2` could clobber scratch
  values still needed by the caller-side lowering sequence.
- Loading indirect call targets from the wrong offset would branch through an
  argument slot instead of the spilled function pointer.
- Removing the F128 `q0` store or the `__trunctfdf2` accumulator update would
  break one of the two result views expected by the old backend.

## Rebuild Guidance

Rebuild this surface around structured call-lowering facts:

1. Keep ABI classification separate from physical emission, but preserve the
   exact AArch64 policy bits listed above.
2. Model outgoing stack layout explicitly, including 16-byte alignment for wide
   scalar classes and the indirect-call function-pointer spill area.
3. Preserve the distinction between exact F128 source recovery and fallback
   double-to-F128 conversion.
4. Keep register-argument staging ordered so source values survive moves into
   final ABI registers.
5. Treat F128 return handling as two observable products until the replacement
   backend has a single structured result-carrier contract.
