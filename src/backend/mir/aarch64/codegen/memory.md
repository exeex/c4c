# AArch64 Memory Legacy Surface

Current target memory work lives in the record-only contract documented in
`records.md`. `MemoryOperand` and `MemoryInstructionRecord` preserve prepared
memory facts for later lowering; this file remains a legacy lowering reference
and does not own the active record contract, load/store selection, assembly
emission, object output, calls, or returns.

This artifact preserves the useful production shape from the removed
`memory.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/memory.rs`.

## Role

The surface implemented AArch64 load, store, address-materialization,
stack-pointer adjustment, over-aligned alloca addressing, and bytewise memcpy
helpers for `ArmCodegen`. It was the main legacy policy point for converting
IR memory operations into the backend's accumulator and scratch-register
conventions.

The old lowering treated `x0` as the primary value accumulator, `x1` as the
secondary value or address register, `x9` as the active memory-address
register, `x10` as the memcpy source-address register, `x11` as the memcpy
byte count, `x12` as the memcpy byte payload register, and `x17` as the
large-immediate scratch register.

## Entry Points

- `emit_store_impl(val, ptr, ty)`: routed F128 stores to the softfloat helper
  and otherwise used the backend default store lowering.
- `emit_load_impl(dest, ptr, ty)`: routed F128 loads to the softfloat helper
  and otherwise used the backend default load lowering.
- `emit_store_with_const_offset_impl(val, base, offset, ty)`: stored a typed
  value through a resolved stack, indirect, or over-aligned alloca base plus a
  constant byte offset.
- `emit_load_with_const_offset_impl(dest, base, offset, ty)`: loaded a typed
  value from a resolved stack, indirect, or over-aligned alloca base plus a
  constant byte offset and stored the result to the IR destination.
- `emit_typed_store_to_slot_impl(instr, ty, slot)` and
  `emit_typed_load_from_slot_impl(instr, slot)`: typed stack-slot load/store
  wrappers around the generic SP-relative helpers.
- `emit_load_ptr_from_slot_impl(slot, val_id)`: resolved a pointer carrier into
  `x9` from an assigned callee-saved register or a stack slot.
- `emit_typed_store_indirect_impl(instr, ty)` and
  `emit_typed_load_indirect_impl(instr)`: emitted typed `[x9]` memory access.
- `emit_add_offset_to_addr_reg_impl(offset)`: adjusted `x9` by a signed byte
  offset using an immediate add/sub when possible or `x17` for large offsets.
- `emit_slot_addr_to_secondary_impl(slot, is_alloca, val_id)`: materialized a
  slot or alloca address into `x1`.
- `emit_gep_direct_const_impl(slot, offset)` and
  `emit_gep_indirect_const_impl(slot, offset, val_id)`: lowered constant GEP
  forms into `x0`.
- `emit_add_imm_to_acc_impl(imm)`: adjusted the accumulator address in `x0`.
- `emit_round_up_acc_to_16_impl`, `emit_sub_sp_by_acc_impl`,
  `emit_mov_sp_to_acc_impl`, `emit_mov_acc_to_sp_impl`, and
  `emit_align_acc_impl(align)`: stack-pointer and accumulator-alignment
  helpers for dynamic stack management.
- `emit_memcpy_load_dest_addr_impl(slot, is_alloca, val_id)` and
  `emit_memcpy_load_src_addr_impl(slot, is_alloca, val_id)`: loaded memcpy
  destination and source addresses into `x9` and `x10`.
- `emit_alloca_aligned_addr_impl(slot, val_id)` and
  `emit_alloca_aligned_addr_to_acc_impl(slot, val_id)`: computed over-aligned
  alloca addresses from SP-relative storage.
- `emit_memcpy_impl_impl(size)`: emitted a bytewise fixed-size memcpy loop.

## Load And Store Dispatch

The ordinary load and store overrides were thin dispatch points. F128 memory
operations were delegated to `f128_softfloat` helpers:

- `f128_emit_store`
- `f128_emit_load`
- `f128_emit_store_with_offset`
- `f128_emit_load_with_offset`

All non-F128 ordinary loads and stores went through default trait lowering.
The constant-offset helpers contained local AArch64 addressing policy for
known stack slots and prepared pointer carriers.

## Constant-Offset Store Behavior

`emit_store_with_const_offset_impl` first materialized the source operand into
`x0`. It then asked backend state to resolve the base value into a `SlotAddr`.
When no address was resolved, the helper emitted nothing.

For over-aligned alloca bases, the helper preserved the source value by moving
`x0` to `x1`, computed the aligned base address into `x9`, added the constant
offset to `x9`, selected the typed source register from `x1`, and emitted:

```asm
<store-instr> <typed-x1>, [x9]
```

For direct stack-slot bases, it folded the constant offset into the stack slot
number, selected the typed source register from `x0`, and used the
SP-relative store helper.

For indirect pointer-slot bases, it moved `x0` to `x1`, loaded the pointer
carrier into `x9`, optionally adjusted `x9` by the offset, selected the typed
source register from `x1`, and emitted the indirect store through `[x9]`.

## Constant-Offset Load Behavior

`emit_load_with_const_offset_impl` similarly resolved the base value into a
`SlotAddr` and emitted nothing when no address was resolved.

For over-aligned alloca bases, it computed the aligned base address into `x9`,
added the offset, parsed the requested load instruction into the actual
mnemonic and destination register, and loaded from `[x9]`.

For direct stack-slot bases, it folded the constant offset into the stack slot
and used the SP-relative load helper with the parsed load mnemonic and
destination register.

For indirect pointer-slot bases, it loaded the pointer carrier into `x9`,
optionally adjusted the address by the offset, parsed the load instruction,
and loaded from `[x9]`.

After any successful typed load, the helper stored the accumulator result to
the destination with `store_x0_to(dest)`.

## Addressing Helpers

The legacy surface had three address families:

- Direct stack-slot addresses folded byte offsets into the `StackSlot` value
  and used existing SP-relative load/store helpers.
- Indirect addresses loaded a pointer carrier from either an assigned
  callee-saved register or an SP-relative slot, then accessed memory through
  `x9`.
- Over-aligned alloca addresses recomputed an aligned address from an
  SP-relative storage slot and the recorded alloca alignment.

`emit_add_offset_to_addr_reg_impl` and `emit_add_imm_to_acc_impl` shared the
same signed-immediate policy. Offsets in `0..=4095` used immediate `add`,
negative offsets in `-4095..=-1` used immediate `sub`, and larger magnitudes
were loaded into a scratch register before a register `add`.

## Stack And GEP Paths

`emit_slot_addr_to_secondary_impl` produced an address in `x1`. For alloca
values it delegated to `emit_alloca_addr("x1", val_id, slot)`. For
register-assigned pointer values it moved the assigned callee-saved register
into `x1`. Otherwise it loaded the pointer from the stack slot.

`emit_gep_direct_const_impl` folded the constant offset into the direct stack
slot and materialized `sp + folded_offset` into `x0`.

`emit_gep_indirect_const_impl` loaded the base pointer into `x0` from an
assigned callee-saved register or stack slot, then adjusted `x0` by the
constant offset when the offset was nonzero.

The stack-pointer helpers were direct assembly shims:

- round the accumulator up to 16 bytes with `add x0, x0, #15` and
  `and x0, x0, #-16`.
- subtract `x0` from `sp`.
- move `sp` into `x0`.
- move `x0` into `sp`.
- align `x0` to an arbitrary power-of-two alignment by adding `align - 1` and
  masking with `-align`.

## Over-Aligned Alloca Addressing

Both aligned-address helpers required
`state.alloca_over_align(val_id)` to return an alignment. The old code treated
absence of that metadata as an internal invariant violation.

The `x9` form:

1. Materialized `sp + slot` into `x9`.
2. Loaded `align - 1` into `x17` and added it to `x9`.
3. Loaded `-align` into `x17`.
4. Masked `x9` with `x17`.

The accumulator form used the same sequence with `x0` and invalidated the
accumulator cache afterward.

## Memcpy Behavior

Memcpy address setup mirrored the general pointer-carrier rules. The
destination helper resolved alloca or pointer carriers into `x9`; the source
helper resolved them into `x10`.

`emit_memcpy_impl_impl(size)` generated a unique loop and done label from
`state.next_label_id()`, loaded the fixed byte count into `x11`, and emitted a
simple byte loop:

```asm
<loop>:
    cbz x11, <done>
    ldrb w12, [x10], #1
    strb w12, [x9], #1
    sub x11, x11, #1
    b <loop>
<done>:
```

The copy was intentionally byte-granular and did not attempt wider moves,
alignment-specific paths, or overlap handling.

## Width And Sign Handling

The memory surface did not directly encode all load/store widths. It delegated
instruction selection to `store_instr_for_type_impl`,
`load_instr_for_type_impl`, `reg_for_type`, and `arm_parse_load`.

The important width rule preserved here is register spelling. Stores selected
the typed source register from either `x0` or `x1`, depending on whether the
value needed to survive address computation. Loads parsed the load instruction
into the concrete mnemonic and destination register before using either
SP-relative or `[x9]` addressing. Any sign-extension or zero-extension
semantics therefore came from the selected load mnemonic and parsed
destination register, not from additional fixup in this file.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `Operand`, `Value`, and `IrType`.
- Backend state address resolution: `state.resolve_slot_addr`,
  `SlotAddr::OverAligned`, `SlotAddr::Direct`, `SlotAddr::Indirect`, and
  `StackSlot`.
- Slot and alloca metadata: `state.alloca_over_align`,
  `state.is_alloca`, and assigned-register metadata through
  `reg_assignments`.
- Register spelling helpers: `callee_saved_name`, `reg_for_type`, and
  `arm_parse_load`.
- Type-to-instruction helpers: `store_instr_for_type_impl` and
  `load_instr_for_type_impl`.
- Operand and result helpers: `operand_to_x0`, `store_x0_to`,
  `emit_load_from_sp`, `emit_store_to_sp`, `emit_add_sp_offset`,
  `emit_alloca_addr`, `load_large_imm`, and `emit_load_imm64`.
- F128 softfloat hooks in `crate::backend::f128_softfloat`.
- Assembly emission through `state.emit` and `state.emit_fmt`.
- Label generation through `state.next_label_id`.
- Register-cache invalidation through `state.reg_cache.invalidate_acc`.

## Hidden Assumptions

- `x0` is the normal value accumulator, and materializing an operand into `x0`
  may clobber any prior value there.
- Stores through over-aligned or indirect addresses must move the source value
  from `x0` to `x1` before address computation can clobber `x0` or `x9`.
- `x9` is available as the active memory-address scratch register for indirect
  accesses.
- `x10`, `x11`, and `w12` are available for bytewise memcpy lowering.
- `x17` is available for large offset and alignment-mask materialization.
- A missing `SlotAddr` resolution means the constant-offset helper emits no
  fallback code.
- Over-aligned alloca metadata must exist before aligned-address emission is
  requested.
- Constant stack-slot offsets are simple byte additions to the recorded
  `StackSlot` value.
- `emit_align_acc_impl` assumes the alignment is valid for an AArch64 bitmask
  sequence using `-align`.
- Bytewise memcpy copies exactly `size` bytes and has no special overlap
  semantics.

## Known Failure Risks For Rebuild

- Dropping the F128 softfloat dispatch would route quad loads and stores
  through scalar memory helpers that do not preserve the legacy behavior.
- Recomputing an over-aligned alloca address without the `align - 1` add and
  `-align` mask would produce an unaligned address.
- Forgetting to preserve the store source in `x1` for indirect and
  over-aligned stores could overwrite the value while computing the address.
- Treating a direct stack slot like an indirect pointer slot would load from
  the wrong address family.
- Treating an indirect pointer slot like a direct stack slot would store into
  the pointer variable rather than the pointed-to memory.
- Losing the immediate add/sub range split could emit invalid AArch64
  immediates for negative or large offsets.
- Omitting `store_x0_to(dest)` after a typed load would leave the loaded value
  only in the accumulator convention.
- Rebuilding typed loads without preserving `arm_parse_load` semantics could
  lose sign-extension or zero-extension encoded in the selected load mnemonic.
- Failing to invalidate the accumulator cache after computing an aligned
  alloca address into `x0` could leave stale cached values.
- Replacing the bytewise memcpy loop with wider accesses without proving
  alignment and overlap assumptions could change legacy semantics.

## Rebuild Guidance

Rebuild this surface around explicit prepared memory facts:

1. Keep direct stack-slot, indirect pointer-slot, and over-aligned alloca
   addressing as distinct paths.
2. Preserve F128 as an explicit helper-routed special case until the final
   AArch64 long-double ABI policy is defined.
3. Make typed load/store width, sign-extension, and zero-extension decisions
   visible at the instruction-selection boundary.
4. Keep address computation scratch-register ownership explicit, especially
   `x9`, `x10`, `x17`, and source-preserving use of `x1`.
5. Treat large signed offsets and alignment masks as materialized-register
   cases rather than assuming immediate encodability.
6. Prove direct, indirect, over-aligned, GEP, dynamic stack, F128, and memcpy
   paths independently during reconstruction.
