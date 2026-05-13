# AArch64 Variadic ABI Legacy Surface

This artifact preserves the useful production shape from the removed
`variadic.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/variadic.rs`.

## Role

The surface implemented AArch64 variadic function operations for `ArmCodegen`:
`va_start`, scalar and aggregate `va_arg`, and `va_copy`. It encoded the
AAPCS64 `va_list` layout, maintained register-save offsets, and chose between
register-save-area reads and stack overflow reads.

The generated assembly used the backend's normal scratch-register convention:
the active `va_list` pointer was loaded into `x1` for `va_arg` paths, scalar
results were returned through `x0`, aggregate results were copied into a
destination pointer loaded into `x4`, and helpers finally stored through the
backend's destination model.

## AAPCS64 `va_list` Layout

The old surface treated `va_list` as a 32-byte record:

```text
+0   __stack    pointer to next stack overflow argument
+8   __gr_top   pointer to top of GP register save area
+16  __vr_top   pointer to top of FP/SIMD register save area
+24  __gr_offs  signed 32-bit offset from __gr_top
+28  __vr_offs  signed 32-bit offset from __vr_top
```

`__gr_offs` and `__vr_offs` started negative and advanced toward zero. A
negative offset meant register-save-area slots remained. Once a path could not
consume the whole argument from the relevant save area, it fell back to the
stack overflow area.

## Entry Points

- `emit_va_start_impl(va_list_ptr)`: initialized a `va_list` record for the
  current variadic function.
- `emit_va_arg_impl(dest, va_list_ptr, result_ty)`: loaded scalar, floating, or
  long-double arguments from the active `va_list`.
- `emit_va_copy_impl(dest_ptr, src_ptr)`: copied the 32-byte `va_list` record.
- `emit_va_arg_struct_impl(dest_ptr, va_list_ptr, size)`: copied aggregate
  variadic arguments from the GP register save area or stack overflow area.
- `load_va_list_ptr` and `load_dest_ptr`: resolved alloca, assigned-register,
  and stack-slot carriers into scratch registers.
- `emit_partial_struct_copy`: copied the partial final aggregate slot with
  4-byte, 2-byte, and 1-byte loads/stores.

## `va_start` Initialization

`va_start` resolved the destination `va_list` pointer into `x0`, then filled
the AAPCS64 fields:

- `__stack` was set to `x29 + current_frame_size + va_named_stack_bytes`.
  Large offsets were materialized through `load_large_imm` before adding to
  `x29`.
- `__gr_top` was set to `sp + va_gp_save_offset + 64`, covering the eight
  8-byte GP argument save slots.
- `__vr_top` was set to `sp + va_fp_save_offset + 128`, covering the eight
  16-byte FP/SIMD save slots. When `general_regs_only` was set, this field was
  stored as zero instead.
- `__gr_offs` was initialized to `-((8 - va_named_gp_count) * 8)`.
- `__vr_offs` was initialized to `-((8 - va_named_fp_count) * 16)`, or zero
  when `general_regs_only` disabled FP/SIMD variadic save usage.

## Scalar `va_arg`

Scalar integer and pointer-like arguments used the GP register save area first:

1. Load signed `__gr_offs` from `[x1, #24]`.
2. If its sign bit was clear, branch to the stack path.
3. Load `__gr_top` from `[x1, #8]`, add the signed offset, advance
   `__gr_offs` by 8 bytes, and load the argument into `x0`.
4. On the stack path, load from `__stack`, advance `__stack` by 8 bytes, and
   leave the value in `x0`.

Floating-point arguments used the FP/SIMD register save area with
`__vr_offs` at `[x1, #28]`, `__vr_top` at `[x1, #16]`, and 16-byte register
save slots. `F32` loaded into `w0`; wider floating values loaded into `x0`.
The stack fallback for the scalar FP path still advanced `__stack` by 8 bytes.

Each scalar path emitted local stack and done labels, then stored `x0` to the
destination slot when one existed.

## F128 Handling

Long double (`F128`) used the FP/SIMD variadic path but converted the loaded
quad value down through the helper call:

```asm
ldr q0, [source]
bl __trunctfdf2
fmov x0, d0
```

The register-save path consumed one 16-byte FP/SIMD slot and advanced
`__vr_offs` by 16. The stack fallback 16-byte-aligned `__stack`, loaded the
quad value, advanced `__stack` by 16, and performed the same helper conversion.
This path invalidated the register cache after the helper call.

## Aggregate `va_arg`

Composite variadic arguments were read from GP argument slots under the AAPCS64
rule that an aggregate must fit entirely in the remaining GP register save
area or be read entirely from the stack overflow area. The old surface computed
`num_slots = ceil(size / 8)` and `total_reg_bytes = num_slots * 8`.

The register path:

1. Loaded signed `__gr_offs`.
2. Added `total_reg_bytes`.
3. Branched to the stack path when the result was greater than zero.
4. Copied every 8-byte slot from `__gr_top + __gr_offs` to the destination.
5. Copied a partial final slot bytewise through the partial-copy helper.
6. Stored the advanced `__gr_offs`.

The stack path set `__gr_offs` to zero to mark GP registers exhausted, loaded
`__stack`, aligned it to 8 bytes, copied the whole aggregate from stack to the
destination, advanced `__stack` by the 8-byte-rounded aggregate size, and
invalidated the register cache at completion.

## `va_copy`

`va_copy` resolved source `va_list` into `x1` and destination `va_list` into
`x0`, then copied exactly 32 bytes with two load/store pairs:

```asm
ldp x2, x3, [x1]
stp x2, x3, [x0]
ldp x2, x3, [x1, #16]
stp x2, x3, [x0, #16]
```

## Dependencies

The removed surface depended on these surrounding concepts:

- IR value carriers and destination slots: `Value`, `state.is_alloca`,
  `state.get_slot`, and `reg_assignments`.
- Scratch register naming through `callee_saved_name`.
- Frame and variadic metadata: `current_frame_size`,
  `va_named_stack_bytes`, `va_gp_save_offset`, `va_fp_save_offset`,
  `va_named_gp_count`, `va_named_fp_count`, and `general_regs_only`.
- Address helpers: `emit_add_fp_offset`, `emit_add_sp_offset`,
  `emit_load_from_sp`, `emit_store_to_sp`, and `load_large_imm`.
- Assembly emission through `state.emit` and `state.emit_fmt`.
- Type classification through `IrType::is_float`, `IrType::is_long_double`,
  and `IrType::F32`.
- Runtime conversion helper symbol `__trunctfdf2` for F128-to-double
  truncation.

## Hidden Assumptions

- The backend has already built the variadic register save areas in the frame
  and recorded their offsets consistently with `va_start`.
- The signed offset fields are negative while register-save slots remain.
- `tbz x2, #63` correctly detects exhaustion for scalar register paths after
  sign-extending the 32-bit offset with `ldrsw`.
- FP/SIMD register-save slots are always 16 bytes, even for `F32` and `F64`.
- GP register-save slots are always 8 bytes.
- Struct variadic arguments are never split between GP register-save area and
  stack overflow area.
- Stack aggregate fallback alignment is 8 bytes; F128 scalar fallback alignment
  is 16 bytes.
- Helper calls can clobber cached registers, so F128 and aggregate paths
  invalidate register-cache state.
- `va_copy` is a raw 32-byte copy of the platform `va_list` record.

## Known Failure Risks For Rebuild

- Initializing `__gr_offs` or `__vr_offs` with positive remaining counts would
  invert the register-vs-stack branch logic.
- Forgetting that FP/SIMD save slots advance by 16 bytes would corrupt
  floating variadic reads after the first argument.
- Allowing aggregate variadic reads to split across register slots and stack
  would violate AAPCS64 and mis-handle boundary cases.
- Failing to zero `__gr_offs` on aggregate stack fallback would let later
  aggregate reads incorrectly reuse exhausted GP slots.
- Treating F128 like ordinary `F64` would miss 16-byte stack alignment, quad
  loads, and the `__trunctfdf2` conversion call.
- Replacing the `va_list` pointer resolution helpers with rendered-name lookup
  would conflict with the markdown-first reconstruction goal of using
  structured backend facts.
- Omitting cache invalidation after helper calls or aggregate copies could
  leave stale values in later generated code.

## Rebuild Guidance

Rebuild this surface around explicit prepared variadic ABI facts:

1. Preserve the 32-byte AAPCS64 `va_list` record shape and signed offset
   semantics.
2. Keep GP and FP/SIMD register-save paths separate, with 8-byte and 16-byte
   slot advances respectively.
3. Model stack fallback as a first-class path, including F128 16-byte alignment
   and aggregate 8-byte alignment.
4. Keep aggregate `va_arg` whole-source selection: all from GP save area or all
   from stack.
5. Treat `__trunctfdf2` as an explicit runtime helper dependency for the legacy
   F128 path until the replacement backend defines its final long-double ABI
   behavior.
6. Prove `va_start`, scalar `va_arg`, aggregate `va_arg`, `va_copy`, register
   exhaustion, stack fallback, and F128 helper behavior independently.
