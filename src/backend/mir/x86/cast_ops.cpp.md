# `cast_ops.cpp` extraction

## Purpose and current responsibility

This file is not the general x86 cast lowering implementation. It is a small routing layer plus a few target-local escape hatches for casts that do not fit the normal accumulator-based path, especially `F128` conversions that must interact with x87 state and stack-backed storage.

Today it owns two things:

- the public x86 hook that forwards generic cast-instruction emission into `emit_cast_instrs_x86`
- a special-case `emit_cast_impl` path for `F128` conversions where plain `RAX`-based lowering is not enough

Everything outside those cases falls through to the broader lowering surface in `lowering/float_lowering.cpp`.

## Important APIs and contract surfaces

Essential surface:

```cpp
void X86Codegen::emit_cast_instrs_impl(IrType from_ty, IrType to_ty);
void X86Codegen::emit_cast_impl(const Value& dest,
                                const Operand& src,
                                IrType from_ty,
                                IrType to_ty);
```

Delegated helpers this file depends on:

```cpp
void emit_cast_instrs_x86(IrType from_ty, IrType to_ty);
void emit_f128_load_to_x87(const Operand& operand);
void emit_f128_to_int_from_memory(const SlotAddr& addr, IrType to_ty);
void emit_f128_st0_to_int(IrType to_ty);
void operand_to_rax(const Operand& op);
void store_rax_to(const Value& dest);
```

State and metadata contracts that materially affect behavior:

```cpp
std::unordered_set<std::uint32_t> f128_direct_slots;
std::optional<StackSlot> get_slot(std::uint32_t value_id) const;
std::optional<std::uint32_t> get_f128_source(std::uint32_t value_id) const;
std::optional<SlotAddr> resolve_slot_addr(std::uint32_t value_id) const;
```

Contract notes:

- `emit_cast_instrs_impl` is a thin target hook. Its only job is to hand control to x86-specific cast instruction selection.
- `emit_cast_impl` assumes the normal scalar contract is `operand_to_rax(src)` -> `emit_cast_instrs_x86(from,to)` -> `store_rax_to(dest)`.
- The file breaks that contract only when `F128` requires x87 stack values or memory-resident source/destination handling.
- `dest.raw` and `src.raw` are used as keys into backend state; correctness depends on side metadata already being populated by earlier lowering and memory-tracking code.

## Dependency direction and hidden inputs

Dependency direction is inward toward broader backend services:

- this file emits through `state.emit(...)` and `state.out.emit_instr_*`
- it reads slot/layout information from `X86CodegenState`
- it relies on `float_lowering.cpp` for the actual reusable conversion primitives
- it relies on reg-cache bookkeeping and `f128_*` metadata to decide whether a value exists as raw bytes, a tracked source slot, or only as a generic operand

Hidden inputs that are easy to miss:

- `f128_direct_slots` changes whether a value may be loaded or converted directly from its stack slot
- `get_f128_source` / `resolve_slot_addr` provide a second chance to recover a stable memory address for `F128 -> int`
- `get_slot(dest.raw)` gates the main `to F128` path; without a stack slot, this file does not own the conversion result format
- the helper-local `fresh_local_label()` uses static process-global state, so label naming is not a pure function of the IR node
- signedness and width classification (`is_signed_ty`, `is_unsigned_ty`, `type_size_bytes`) silently choose between sign-extension, split unsigned handling, and fallback behavior

## Responsibility buckets

### 1. Public hook forwarding

`emit_cast_instrs_impl` exists only to route generic backend requests into the x86-specific cast emitter. There is no policy here.

### 2. `to F128` via x87 plus stack slot materialization

When `to_ty == F128` and the source is not already `F128` or `I128`, this file bypasses the generic path and tries to materialize a real x87 long-double value into the destination slot.

Current subcases:

- `F64 -> F128`: spill the `RAX` payload to the stack, `fldl`, then store the long double to the destination slot
- `F32 -> F128`: same shape with `flds`
- signed or default-integer-like source -> `F128`: move through `RAX`, widen to `I64` if needed, then `fildq`
- unsigned source -> `F128`: use a split path when the high bit is set, then add a baked `2^63`-style constant through x87

This path also:

- stores the x87 result to the destination stack slot
- reloads a truncated `double` representation back into `RAX`
- marks the destination in `f128_direct_slots`
- updates the accumulator cache for the destination id

That means the function is doing both conversion and value-representation bookkeeping.

### 3. `F128 -> F64/F32`

This path loads the source into x87, stores either 8 or 4 bytes to a temporary stack slot, moves the raw result into `RAX/EAX`, invalidates the accumulator cache, and stores the scalar result normally.

### 4. `F128 -> integer/pointer` with memory-sensitive fast path

This path prefers converting from memory instead of first rebuilding an x87 value from a generic operand:

- first choice: direct stack slot if the operand id is in `f128_direct_slots`
- second choice: tracked original `F128` source address via `get_f128_source` + `resolve_slot_addr`
- fallback: generic `emit_f128_load_to_x87(src)` then `emit_f128_st0_to_int(to_ty)`

This bucket is mostly a policy wrapper around helpers defined elsewhere.

### 5. Everything else fallback

For all non-special cases, the file explicitly uses the default local cast pipeline:

```cpp
operand_to_rax(src);
emit_cast_instrs_x86(from_ty, to_ty);
store_rax_to(dest);
```

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- `F128 -> int` prefers direct-memory conversion when stack-slot provenance is known
- `to F128` avoids the generic path and writes directly into the destination slot when one exists

These are performance and representation fast paths, but they also serve as correctness paths because x87 long double values do not fit the plain accumulator convention.

### Legacy compatibility paths

- x87-based `F128` handling is explicitly legacy-target compatibility logic
- unsigned integer to `F128` uses a sign-test split plus an injected constant instead of a single generalized helper
- `F128` results are partially mirrored back through `RAX` after stack-slot storage so the rest of the backend can keep using accumulator-oriented conventions

### Overfit pressure

The strongest rebuild pressure is not testcase-shaped branching by name, but representation-shaped coupling:

- `emit_cast_impl` knows too much about slot existence, x87 staging, cache maintenance, and provenance recovery
- `F128` cases are mixed into one dispatcher instead of living behind a dedicated value-representation boundary
- the unsigned-to-`F128` split logic is duplicated policy that conceptually belongs with the other floating conversion helpers
- the file’s behavior depends on side maps (`f128_direct_slots`, `f128_load_sources`) that are not visible in the function signature

This is not yet a clean “cast ops” unit. It is a compatibility bridge between a generic cast API and the backend’s special `F128` storage model.

## Rebuild ownership

This file should own:

- the narrow public routing surface from generic cast hooks into x86 lowering
- at most a thin policy layer that selects between reusable cast backends

This file should not own:

- x87 long-double materialization details
- stack-slot provenance recovery for `F128`
- reg-cache mutation policy for special representations
- duplicated unsigned/signed conversion tricks that belong in reusable lowering helpers
- mixed concerns of conversion, storage layout, and result bookkeeping in one dispatcher
