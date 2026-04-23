# `memory.cpp` extraction

## Purpose and current responsibility

This file is the x86 backend's small-memory-operations and address-materialization helper layer. It turns already-lowered MIR memory intents into concrete textual x86 assembly by coordinating:

- scalar load/store emission
- `f128` load/store special handling through x87-oriented helpers
- stack-slot and indirect-pointer address formation
- GEP-style pointer arithmetic in accumulator/secondary registers
- stack-pointer alignment and movement helpers
- `memcpy` argument setup plus `rep movsb`
- segment-prefixed load/store forms for `FS` and `GS`

It does not decide memory semantics at a high level. Its job is to pick the concrete emission path once the surrounding backend has already decided type, address base, offset, and storage class.

## Important APIs and contract surfaces

The public surface here is a set of `X86Codegen` implementation methods. The file is mostly policy glue around lower-level emitter helpers and `state` queries.

```cpp
void X86Codegen::emit_store_impl(const Operand& val, const Value& ptr, IrType ty);
void X86Codegen::emit_load_impl(const Value& dest, const Value& ptr, IrType ty);
void X86Codegen::emit_store_with_const_offset_impl(
    const Operand& val, const Value& base, std::int64_t offset, IrType ty);
void X86Codegen::emit_load_with_const_offset_impl(
    const Value& dest, const Value& base, std::int64_t offset, IrType ty);
```

Contract shape:

- callers provide already-typed MIR values and a base pointer identity
- this layer assumes `resolve_slot_addr()` can classify the base as direct stack, indirect pointer, or over-aligned alloca
- results are emitted directly into backend output; there is no abstract memory instruction result
- integer/pointer-sized work is centered on `%rax`, `%rcx`, `%rdx`
- loads must finish by storing `%rax` back into backend value state with `store_rax_to(dest)`

Important helper surfaces inside the file:

```cpp
void X86Codegen::emit_typed_store_to_slot_impl(const char* instr, IrType ty, StackSlot slot);
void X86Codegen::emit_typed_load_from_slot_impl(const char* instr, StackSlot slot);
void X86Codegen::emit_typed_store_indirect_impl(const char* instr, IrType ty);
void X86Codegen::emit_typed_load_indirect_impl(const char* instr);
void X86Codegen::emit_alloca_addr_to(const char* reg, std::uint32_t val_id, std::int64_t offset);
void X86Codegen::emit_seg_load_impl(const Value& dest, const Value& ptr, IrType ty, AddressSpace seg);
void X86Codegen::emit_seg_store_impl(const Operand& val, const Value& ptr, IrType ty, AddressSpace seg);
```

These helpers encode calling conventions internal to the backend:

- `%rax` is the primary accumulator
- `%rcx` is the secondary address register
- `%rdx` is the preserved value register for indirect stores
- `%rdi/%rsi/%rcx` are used for `memcpy`
- segment operations require non-default `AddressSpace`; default is treated as programmer error

## Dependency direction and hidden inputs

Direct dependencies flow downward into `X86Codegen` state and text emitters, not upward into MIR policy.

Hidden inputs this file relies on heavily:

- `state.resolve_slot_addr(value_id)` decides whether an access is `Direct`, `Indirect`, or `OverAligned`
- `state.assigned_reg_index(value_id)` silently changes whether an address/value comes from a physical register or a stack slot
- `mov_load_for_type()` and `mov_store_for_type()` define width-specific opcode selection
- `reg_for_type()` decides which register name variant matches the type width
- `state.alloca_over_align(value_id)` changes address materialization semantics
- `state.get_f128_constant_words()`, `state.f128_direct_slots`, and `state.track_f128_load()` carry `f128`-specific side tables
- `state.reg_cache.invalidate_*()` must be called correctly after pointer math and stack-pointer writes

This means the file is not a self-contained memory layer. It is a backend coordination point whose behavior depends on register allocation, slot classification, type lowering, and f128 bookkeeping already having been done elsewhere.

## Responsibility buckets

### 1. Generic scalar load/store routing

- `emit_store_impl` / `emit_load_impl` dispatch to default or `f128` paths
- `emit_*_with_const_offset_impl` handles the real branching for direct stack, indirect pointer, and over-aligned alloca access
- scalar stores first move the source into `%rax`
- scalar loads leave the result in `%rax` and then materialize backend state with `store_rax_to`

### 2. Address-shape-specific lowering

- `Direct`: emit `%rbp`-relative memory operands straight from the stack slot
- `Indirect`: use an assigned physical register if available, otherwise load the pointer into `%rcx`
- `OverAligned`: reconstruct an aligned effective address before performing indirect access

This is the main dispatch responsibility in the file.

### 3. `f128` compatibility path

- constant `f128` stores can serialize raw bytes directly
- direct-slot `f128` stores use `fld` + `fstpt`
- fallback `f128` store path goes through `%rax` and x87 helper routines
- `f128` loads use `fldt`, then a finish helper, then optional load tracking

This is not a normal typed-memory path. It is a compatibility lane for a type the scalar helpers cannot encode directly.

### 4. Address and pointer arithmetic helpers

- alloca address materialization with optional over-alignment masking
- slot-to-register address loading
- constant-offset GEP emission
- immediate add helpers on accumulator and secondary registers
- stack-pointer round-up, move, and subtract operations

These functions expose the file as an address-manipulation utility module, not just a load/store emitter.

### 5. Bulk copy and segment-memory helpers

- `memcpy` setup uses `%rdi`, `%rsi`, `%rcx`, then emits `rep movsb`
- segment loads/stores inject `%fs:` or `%gs:` prefixes for pointer-based and RIP-relative symbol forms

These are semantically separate from ordinary stack-slot memory traffic but are housed here anyway.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- indirect accesses skip stack reloads when the base pointer already has an assigned physical register
- direct stack-slot accesses avoid temporary address materialization
- `f128` constant stores bypass x87 load/store traffic by writing raw bytes

### Compatibility paths

- `f128` handling is a dedicated compatibility path built around x87 and side tables
- segment-prefix helpers are a compatibility path for explicit non-default address spaces
- over-aligned alloca handling exists because plain `%rbp + offset` addressing is not enough for some slots

### Overfit pressure

- the file mixes generic memory operations, stack-pointer utilities, `memcpy`, segment memory, and `f128` quirks in one place
- register choices are hardcoded by convention (`rax/rcx/rdx/rdi/rsi`) rather than surfaced as explicit abstractions
- `f128` logic branches on backend bookkeeping details (`constant_words`, `direct_slots`) that make the path shape-sensitive
- string comparisons like `std::string(instr) == "movl"` are local convenience decisions that couple opcode spelling to register selection

The main rebuild risk is preserving all these ad hoc compatibility branches without re-asserting clearer ownership boundaries.

## What a rebuild should and should not own

A rebuild should own:

- a clear memory-emission surface for typed load/store and address formation
- explicit separation between generic memory lowering and special compatibility lanes
- address-shape handling that does not hide core policy inside scattered register moves

A rebuild should not own:

- unrelated stack-pointer choreography mixed into generic memory helpers
- `f128`, segment-memory, and bulk-copy behavior folded into one undifferentiated utility file
- new testcase-shaped special cases that extend the current hidden-convention style
