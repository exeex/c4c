# `atomics_intrinsics_lowering.cpp`

## Purpose And Current Responsibility

This file is the x86 MIR-to-assembly lowering bucket for two different concerns that ended up co-located:

- atomic memory operations (`load`, `store`, `rmw`, `cmpxchg`, `fence`)
- a wide intrinsic dispatcher for x86-specific scalar, fence, cache, CRC, AES, and 128-bit SIMD operations

It is not defining the frontend semantic model for atomics or intrinsics. Its job is to turn already-selected MIR operations and `IntrinsicOp` enums into concrete x86 instruction sequences by driving `X86Codegen::state.emit(...)` and existing register/operand helpers.

The file behaves as a late backend lowering table plus a small amount of hand-written expansion logic when one opcode needs multiple instructions or temporary register choreography.

## Important APIs And Contract Surfaces

Primary owned entry points:

```cpp
void X86Codegen::emit_atomic_rmw_impl(const Value& dest,
                                      AtomicRmwOp op,
                                      const Operand& ptr,
                                      const Operand& val,
                                      IrType ty,
                                      AtomicOrdering ordering);

void X86Codegen::emit_atomic_cmpxchg_impl(const Value& dest,
                                          const Operand& ptr,
                                          const Operand& expected,
                                          const Operand& desired,
                                          IrType ty,
                                          AtomicOrdering success_ordering,
                                          AtomicOrdering failure_ordering,
                                          bool returns_bool);

void X86Codegen::emit_intrinsic_impl(const std::optional<Value>& dest,
                                     const IntrinsicOp& intrinsic,
                                     const std::vector<Operand>& args);
```

Observed contract assumptions:

- `dest` is usually a storage location pointer-like value for vector-style intrinsics, but a scalar destination for atomic/scalar-returning paths.
- atomic ordering is mostly accepted as API surface but only partially honored:
  - `emit_atomic_store_impl` emits `mfence` only for `SeqCst`
  - `emit_fence_impl` emits `mfence` for every non-`Relaxed`
  - `emit_atomic_rmw_impl`, `emit_atomic_cmpxchg_impl`, and `emit_atomic_load_impl` ignore their ordering arguments in this file
- vector intrinsic arguments are usually treated as addresses to 128-bit memory blobs, not SSA-native vector registers
- immediate-bearing intrinsics rely on `Operand::immediate`; missing immediates silently become `0` through `operand_to_imm_i64`

Local mapping helpers define much of the compatibility surface:

```cpp
const char* sse_binary_mnemonic(IntrinsicOp intrinsic);
const char* sse_unary_imm_mnemonic(IntrinsicOp intrinsic);
const char* sse_shuffle_mnemonic(IntrinsicOp intrinsic);
```

These are effectively partial opcode tables. Unsupported enum values are treated as logic errors.

## Dependency Direction And Hidden Inputs

Dependency direction is one-way into backend codegen infrastructure:

- depends on `atomics_intrinsics_lowering.hpp` for the class/member declarations
- depends on `X86Codegen` helpers such as `operand_to_rax`, `operand_to_reg`, `store_rax_to`, `mov_load_for_type`, `mov_store_for_type`, `type_suffix`, `reg_for_type`, and `emit_x86_atomic_op_loop`
- depends on `state.emit(...)`, `state.out.emit_instr_imm_reg(...)`, and `state.reg_cache.invalidate_all()`
- depends on enum stability for `IntrinsicOp`, `AtomicRmwOp`, `AtomicOrdering`, and `IrType`

Hidden inputs and backend assumptions:

- fixed scratch register conventions: many paths hard-code `%rax`, `%rcx`, `%rdx`, `%xmm0`, `%xmm1`
- ABI/frame assumptions: `FrameAddress`, `ReturnAddress`, and `ThreadPointer` directly read `%rbp`, `8(%rbp)`, and `%fs:0`
- backend policy that SIMD values live in memory and are loaded/stored with `movdqu` instead of being tracked in vector registers across MIR values
- cache invalidation correctness depends on callers honoring `reg_cache.invalidate_all()` boundaries after raw assembly emission

## Responsibility Buckets

### Core Lowering

- atomic read/modify/write lowering using `xadd`, `xchg`, `cmpxchg`, and helper-loop expansion for non-native RMW shapes
- atomic loads/stores/fences for integer and float payload types
- scalar intrinsic lowering for `sqrt`, `fabs`, frame/return/thread pointer access, and CRC32
- vector load/store/transform wrappers that marshal memory operands into `%xmm*`, execute one instruction, then spill back

### Optional Fast Paths

- direct `lock xadd` and `xchg` paths instead of always using compare-exchange loops
- zero-immediate float materialization via `xorps`/`xorpd`
- direct mnemonic-table lowering for groups of SSE/AES operations that share the same load-op-store shape

### Legacy Compatibility Paths

- broad monolithic `switch (intrinsic)` with many unrelated op families in one function
- memory-backed 128-bit intrinsic handling even for operations that conceptually want vector register values
- orderings collapsed to coarse `mfence` handling rather than per-ordering semantics throughout
- immediate extraction defaulting to zero, which is permissive rather than defensive

### Overfit Pressure

- every new `IntrinsicOp` tends to be added as another `case` in the central switch with custom register choreography
- grouped helpers are shape-based around current opcode families, which encourages adding more table entries instead of separating ownership by subsystem
- ABI- and register-specific assumptions are embedded inline, making targeted fixes easy but architectural cleanup harder

## Notable Fast Paths, Compatibility Paths, And Risks

Atomic notes:

- `Add` and `Xchg` get dedicated instruction sequences; `Sub`/`And`/`Or`/`Xor`/`Nand` are pushed into `emit_x86_atomic_op_loop`
- `TestAndSet` is special-cased as byte exchange plus zero-extension
- `cmpxchg` optionally returns success as boolean by rewriting `%al`

Intrinsic notes:

- several 128-bit families are normalized into helper patterns:
  - binary vector op
  - unary vector op with immediate
  - shuffle op with immediate
- many other intrinsics stay one-off in the central switch because they do not fit those templates exactly
- non-temporal stores use a helper but still encode different payload-loading conventions per opcode

Compatibility and rebuild risk:

- this file mixes atomic semantics with unrelated ISA feature lowering, so change pressure from one area threatens the others
- the switch is acting as both opcode registry and implementation site
- the backend-visible contract is broader than the filename suggests; a rebuild cannot treat this as “just atomics”

## Rebuild Ownership

This file should own only final x86 instruction selection for already-lowered atomic and x86 intrinsic operations, with narrow helpers for register/materialization mechanics.

It should not own:

- the full registry of unrelated intrinsic families in one monolithic implementation unit
- ABI/thread/frame policy decisions mixed beside SIMD lowering tables
- memory-representation policy for all vector-like values
- silent compatibility behavior that hides missing immediates or ignores most atomic ordering distinctions
