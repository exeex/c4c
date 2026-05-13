# AArch64 Prologue Legacy Surface

This artifact preserves the useful production shape from the removed
`prologue.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/prologue.rs`.

## Role

The surface implemented the `ArmCodegen` frame setup, teardown, parameter
storage, and parameter-reference behavior. It connected stack-space
calculation, register allocation, variadic save-area layout, callee-saved
register preservation, ABI parameter classification, and type-specific stack
loads/stores.

The code assumed surrounding AArch64 emit helpers already owned instruction
formatting for stack-relative loads and stores, frame adjustment, alloca-slot
tracking, and physical register naming. This file supplied the sequencing and
ABI policy that made those helpers cooperate at function entry and exit.

## Entry Points

- `calculate_stack_space_impl(func)`: pre-scans inline assembly clobbers,
  chooses available register pools, runs register allocation, lays out common
  stack slots, then reserves variadic save areas and callee-saved spill slots.
- `aligned_frame_size_impl(raw_space)`: rounds a raw frame size up to the
  required 16-byte AArch64 frame alignment.
- `emit_prologue_impl(func, frame_size)`: records current return/frame state,
  emits the generic AArch64 prologue, then saves used callee-saved registers
  with paired `stp` stores where possible.
- `emit_epilogue_impl(frame_size)`: restores callee-saved registers before
  emitting the generic AArch64 epilogue.
- `emit_store_params_impl(func)`: saves variadic argument registers, classifies
  parameters, discovers parameter alloca slots, pre-stores selected
  register-allocated parameter refs, then stores GP, FP, and stack parameters.
- `emit_param_ref_impl(dest, param_idx, ty)`: materializes a parameter reference
  from a pre-store, alloca slot, ABI register, FP register, or caller stack
  location.
- `emit_epilogue_and_ret_impl(frame_size)`: combines callee-saved restoration,
  generic epilogue emission, and final `ret`.
- `store_instr_for_type_impl(ty)` and `load_instr_for_type_impl(ty)`: map IR
  types to target store/load mnemonics or pseudo-mnemonics used by surrounding
  AArch64 emit helpers.

## Stack Layout Behavior

Stack-space calculation used the shared `calculate_stack_space_common` helper
with 16-byte frame alignment. Individual allocations were placed at an
effective alignment of at least 8 bytes, and allocation sizes were rounded to
at least one 8-byte slot.

Register allocation was run after an inline-assembly callee-saved clobber
prescan. Non-variadic functions could use the normal AArch64 callee-saved and
caller-saved pools, filtered by inline-asm clobbers. Variadic functions used no
base callee-saved pool and no caller-saved pool. Any detected `F128` operation
also cleared the caller-saved pool, presumably to avoid fragile temporary reuse
around software or multi-register lowering.

Variadic functions reserved ABI save areas after common stack layout:

- the GP register save area was 8-byte aligned, recorded as
  `va_gp_save_offset`, and reserved 64 bytes;
- when floating-point registers were allowed, the FP save area was 16-byte
  aligned, recorded as `va_fp_save_offset`, and reserved 128 bytes;
- named GP and FP register counts were computed from ABI parameter classes and
  capped at 8;
- the hidden sret pointer in `x8` was not counted as consuming a GP argument
  register;
- named stack-argument byte usage was recorded through
  `named_params_stack_bytes`.

Used callee-saved registers were placed after the variadic areas, with the
callee-save area aligned to 8 bytes and recorded as `callee_save_offset`.

## Prologue And Epilogue Behavior

The prologue path set `current_return_type`, `current_frame_size`, and reset
`frame_base_offset` before delegating to `emit_prologue_arm`. After the base
prologue, it saved all `used_callee_saved` registers at `callee_save_offset`.
Pairs were stored with `stp` through `emit_stp_to_sp`; a trailing single
register was stored with `str` through `emit_store_to_sp`.

The plain epilogue and the return epilogue both restored callee-saved registers
before the frame teardown. The return-specific helper appended `ret` after
`emit_epilogue_arm`.

## Parameter Storage Behavior

Parameter storage began by saving variadic registers when `func.is_variadic`
was true. It then classified parameters through the backend call ABI config and
recorded the classes, parameter count, variadic flag, and per-parameter alloca
slots in backend state.

The surface included a pre-store optimization for GP parameters whose alloca
slot had been eliminated but whose `ParamRef` destination was assigned to a
callee-saved register. Those parameters were moved from the ABI argument
register into the final callee-saved register at the start of the prologue so
later codegen could clobber the ABI register safely. The optimization was
guarded by several conditions:

- only GP register parameter classes were eligible;
- parameters with alloca slots were skipped because the normal store path
  handled them;
- only physical `x20` through `x28` were considered safe final registers;
- if multiple parameter refs shared one physical register, pre-store was
  skipped to avoid extending overlapping lifetimes back to function entry;
- the first sret parameter moved from `x8` instead of consuming `x0`;
- other sret-shifted parameters adjusted their ABI GP index before selecting
  from `ARM_ARG_REGS`.

After pre-store handling, the surface delegated to GP, FP, and stack parameter
storage helpers in that order.

## Parameter Reference Behavior

`emit_param_ref_impl` was a late materialization path for `ParamRef`
instructions. It first skipped parameters that had already been pre-stored into
their register-allocated destination.

If the parameter had a recorded alloca slot, it loaded from that slot using the
load instruction selected for the alloca type and then stored the result into
the destination value. Otherwise, it used the saved ABI parameter class:

- `IntReg` loaded from the correct argument register, with the first sret
  parameter coming from `x8` and later GP indices shifted as needed.
- `FloatReg` moved from `sN` for `F32` and from `dN` for wider floating-point
  values using `fmov` through `w0` or `x0`.
- `StackScalar` loaded from `current_frame_size + offset`, reflecting caller
  stack arguments located above the current frame.

Any unrecognized or unsupported class fell through without emission.

## Type Load And Store Policy

Stores delegated to the target `str_for_type` helper. Loads used explicit
sign/zero-extension policy:

- `I8` used `ldrsb`.
- `U8` used `ldrb`.
- `I16` used `ldrsh`.
- `U16` used `ldrh`.
- `I32` used `ldrsw`.
- `U32` and `F32` used the pseudo/load selector `ldr32`.
- all other types used `ldr64`.

The surrounding helpers were expected to parse pseudo-mnemonics such as
`ldr32` and select the concrete register view and load form.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `IrFunction`, `Instruction`, `Value`, and `IrType`.
- Common frame layout helpers: `calculate_stack_space_common` and
  `find_param_alloca`.
- Call ABI classification: `ParamClass`, `classify_params`,
  `named_params_stack_bytes`, GP/FP register counts, and sret state.
- Register allocation: physical register identifiers, inline-asm callee-saved
  prescan, clobber merging, liveness caching, and `used_callee_saved`.
- AArch64 target constants: `ARM_CALLEE_SAVED`, `ARM_CALLER_SAVED`,
  `ARM_ARG_REGS`, and `callee_saved_name`.
- AArch64 emit helpers: frame prologue/epilogue emission, stack-relative
  paired and scalar stores, stack loads, `store_x0_to`, load-mnemonic parsing,
  and type-to-register-name conversion.
- Backend state fields for parameter classes, parameter alloca slots,
  pre-stored parameter indices, variadic save offsets, named variadic counts,
  frame size, return type, and sret use.

## Hidden Assumptions

- Stack slots returned by shared layout can be addressed from `sp` after the
  frame prologue has established the final frame size.
- `used_callee_saved` order is stable between stack-size calculation and
  prologue/epilogue emission.
- Variadic GP and FP save areas are placed in the local frame with offsets
  later understood by `va_start` lowering.
- The hidden sret pointer is carried in `x8`, not in the normal `x0` through
  `x7` GP argument sequence.
- Pre-storing a parameter into a callee-saved destination is only safe when no
  other parameter reference shares that physical register.
- Caller-saved parameter destinations are unsafe for the pre-store
  optimization because normal codegen scratch use can overlap them.
- `current_frame_size` is available when stack-passed parameter refs are
  emitted.
- The load pseudo-instructions `ldr32` and `ldr64` are understood by
  `arm_parse_load` and downstream emit helpers.

## Known Failure Risks For Rebuild

- Variadic accounting can silently go wrong if the sret `x8` pointer is counted
  as a normal GP argument.
- Rebuilding the pre-store optimization without the shared-register guard can
  create incorrect function-entry lifetimes for parameters whose original live
  ranges did not overlap.
- Saving callee-saved registers before `used_callee_saved` is finalized would
  miss registers introduced by inline assembly scratch allocation or register
  allocation.
- Clearing caller-saved registers for `F128` operations is broad but
  intentional in the old surface; narrowing it requires proof that all F128
  lowering paths preserve temporary-register assumptions.
- Stack-passed parameters use `frame_size + offset`; using the raw ABI offset
  directly would address inside the callee frame instead of the caller argument
  area.
- FP parameter refs move through integer registers with `fmov`; width selection
  must match the IR type or the upper bits and NaN payload behavior can change.
- Type load selection encodes sign extension for signed narrow integers. A
  generic unsigned load for `I8`, `I16`, or `I32` would change semantics.
- The prologue save path assumes 8-byte slots for every callee-saved GP
  register and uses paired stores only when two adjacent saved registers are
  available.

## Rebuild Guidance

Rebuild this behavior from ABI responsibilities rather than line-for-line:

1. Establish stack-layout ownership for common locals, variadic save areas, and
   callee-saved spills.
2. Rebuild register-allocation setup with explicit inline-asm clobber handling
   and F128 temporary-register policy.
3. Rebuild prologue/epilogue save and restore around the finalized
   `used_callee_saved` set.
4. Rebuild parameter storage from ABI classes, including variadic saves and the
   callee-saved pre-store optimization.
5. Rebuild `ParamRef` materialization with sret, FP width, alloca-slot, and
   stack-passed-argument cases covered by targeted tests.
