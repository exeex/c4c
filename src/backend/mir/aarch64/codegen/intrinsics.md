# AArch64 Intrinsics Legacy Surface

This artifact preserves the useful production shape from the removed
`intrinsics.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/intrinsics.rs`.

## Role

The surface implemented AArch64 lowering for selected platform intrinsics on
`ArmCodegen`.

It covered memory-ordering intrinsics, cache and pause hints, non-temporal
stores, unaligned 128-bit vector loads and stores, a small subset of
SSE-equivalent NEON vector operations, CRC32 instructions, frame/return/thread
address builtins, and scalar floating-point unary operations.

The file also documented the intended binary128 soft-float helper boundary, but
the visible old surface did not contain concrete F128 call-emission routines.

## Entry Points

- `emit_neon_binary_128(dest_ptr, args, neon_inst)`: loads two 128-bit vector
  operands, emits one NEON binary instruction on `v0` and `v1`, and stores the
  resulting `q0` vector through `dest_ptr`.
- `store_scalar_dest(dest, reg)`: stores a scalar result from `x0`, `w0`, or a
  caller-selected register into the destination stack slot.
- `emit_f64_unary_neon(dest, args, op_inst)`: bridges an F64 payload from `x0`
  into `d0`, emits a unary FP instruction, moves the raw result bits back to
  `x0`, and stores the scalar result.
- `emit_f32_unary_neon(dest, args, op_inst)`: bridges an F32 payload from `w0`
  into `s0`, emits a unary FP instruction, moves the raw result bits back to
  `w0`, and stores the scalar result.
- `emit_nontemporal_store(dest_ptr, args, save_reg, val_reg)`: preserves a
  scalar value in a temporary register while rematerializing the destination
  pointer, then emits a plain store.
- `emit_intrinsic_arm(dest, op, dest_ptr, args)`: dispatches each supported
  `IntrinsicOp` to the matching AArch64 sequence.

## Lowering Groups

### Barriers, Hints, And Cache Maintenance

The old mappings were direct instruction selections:

- `Lfence` and `Mfence`: `dmb ish`
- `Sfence`: `dmb ishst`
- `Pause`: `yield`
- `Clflush`: materialize the address in `x0`, then emit `dc civac, x0`

`Clflush` used ARM clean-and-invalidate-to-point-of-coherency as the closest
available operation rather than a true x86 cache-line flush equivalent.

### Non-Temporal And Unaligned Memory Intrinsics

Scalar non-temporal stores were lowered as ordinary stores:

- `Movnti`: save `w0` in `w9`, rematerialize the destination pointer, and
  `str w9, [x0]`.
- `Movnti64`: save `x0` in `x9`, rematerialize the destination pointer, and
  `str x9, [x0]`.

The 128-bit store/load-style intrinsics used `q0`:

- `Movntdq` and `Movntpd`: load `q0` from the source pointer and store it to
  `dest_ptr`.
- `Loaddqu`: load `q0` from the source pointer and store the vector result to
  `dest_ptr`.
- `Storedqu`: load `q0` from the source-data pointer and store it through
  `dest_ptr`.

The old route did not use special streaming or non-temporal AArch64
instructions; these were semantic placeholders around ordinary loads/stores.

### NEON Vector Operations

Most 128-bit vector binary operations shared `emit_neon_binary_128`, which used
this shape:

```asm
ldr q0, [x0]
ldr q1, [x1]
<neon> v0.16b, v0.16b, v1.16b
str q0, [x0]
```

The shared helper covered:

- `Pcmpeqb128`: `cmeq v0.16b, v0.16b, v1.16b`
- `Psubusb128`: `uqsub v0.16b, v0.16b, v1.16b`
- `Psubsb128`: `sqsub v0.16b, v0.16b, v1.16b`
- `Por128`: `orr v0.16b, v0.16b, v1.16b`
- `Pand128`: `and v0.16b, v0.16b, v1.16b`
- `Pxor128`: `eor v0.16b, v0.16b, v1.16b`

`Pcmpeqd128` was special-cased to use 32-bit lanes:

```asm
cmeq v0.4s, v0.4s, v1.4s
```

`SetEpi8` and `SetEpi32` broadcast a scalar into all lanes with `dup v0.16b,
w0` or `dup v0.4s, w0`, then stored `q0` through the destination pointer.

`Pmovmskb128` emulated x86 byte-sign-bit extraction with NEON: load `q0`, shift
each byte right by 7, multiply by per-byte bit-position constants, horizontally
sum the low and high 8-byte halves, and combine them into a 16-bit mask in
`w0`.

### CRC And Builtins

CRC32 intrinsics used the AArch64 CRC extension mnemonics after preserving the
initial CRC value in a fixed temporary register:

- `Crc32_8`: `crc32cb w9, w9, w0`
- `Crc32_16`: `crc32ch w9, w9, w0`
- `Crc32_32`: `crc32cw w9, w9, w0`
- `Crc32_64`: `crc32cx w9, w9, x0`

The resulting CRC was moved through `x0` and stored as the scalar result.

Builtin address intrinsics used fixed AArch64 locations:

- `FrameAddress`: `mov x0, x29`
- `ReturnAddress`: `ldr x0, [x29, #8]`
- `ThreadPointer`: `mrs x0, tpidr_el0`

### Scalar FP Unary Operations

The old route implemented `SqrtF64`, `SqrtF32`, `FabsF64`, and `FabsF32`
through `fsqrt` or `fabs`.

F64 values crossed through `fmov d0, x0`; F32 values crossed through
`fmov s0, w0`. Results were moved back to `x0` or `w0` before stack storage,
matching the backend's raw-bit accumulator convention.

### Unsupported X86-Only Intrinsics

A large group of x86-only SSE, AES-NI, CLMUL, shuffle, pack/unpack, insert,
extract, and conversion intrinsics were explicitly treated as not expected on
the AArch64 route. If a destination pointer was present, the old lowering wrote
zero to the 128-bit destination with:

```asm
add x9, sp, #<slot>
stp xzr, xzr, [x9]
```

The old comment suggested a runtime trap might be preferable to silent zeros in
a future rebuild.

## Register Conventions

- `x0`/`w0`: universal operand accumulator, pointer carrier, scalar result
  carrier, and scratch input to many instruction sequences.
- `x1`: second vector operand pointer for binary NEON operations.
- `q0`/`v0`: first vector operand and vector result.
- `q1`/`v1`: second vector operand or vector constants.
- `s0`/`d0`: scalar FP operation register.
- `w9`/`x9`: saved scalar temporary for non-temporal stores and CRC input.
- `x29`: frame pointer used for `FrameAddress` and `ReturnAddress`.
- `x30`: link register was intentionally not read directly for
  `ReturnAddress`; the old route loaded from `[x29, #8]`.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `IntrinsicOp`, `Operand`, and `Value`.
- Backend operand materialization through `operand_to_x0`.
- Pointer materialization through `load_ptr_to_reg`.
- Stack slot queries through `state.get_slot` and alloca detection through
  `state.is_alloca`.
- Address emission through `emit_alloca_addr`, `emit_load_from_sp`, and
  `emit_store_to_sp`.
- Backend assembly emission through `state.emit` and `state.emit_fmt`.
- AArch64 NEON/SIMD, CRC, barrier, cache-maintenance, and system-register
  instruction availability.

## Hidden Assumptions

- 128-bit SIMD values are represented by memory pointers; vector operations
  load from pointer operands and store through `dest_ptr`.
- Scalar floating-point values travel through integer registers as raw IEEE
  bits until explicitly moved into `s0` or `d0`.
- Re-materializing a destination pointer through `x0` is allowed after saving a
  scalar source value in `w9` or `x9`.
- `emit_neon_binary_128` assumes `args[1]` can be converted to an address in
  `x1`, either from an existing stack slot/alloca or by first materializing it
  in `x0`.
- `Pmovmskb128` relies on byte-lane multiplication and horizontal addition
  producing two 8-bit half masks that can be ORed into a 16-bit result.
- CRC32 lowering assumes the target CPU enables the AArch64 CRC extension or
  that later feature gating will own any fallback/trap behavior.
- Non-temporal x86 store intrinsics were not given true non-temporal AArch64
  semantics in this old path.
- X86-only intrinsic calls reaching this path were assumed to be unreachable or
  already dead-code-eliminated for cross-compiled AArch64 code.
- The binary128 section was documentation-only in this surface; concrete F128
  soft-float lowering belongs to a separate rebuild boundary.

## Known Failure Risks For Rebuild

- Treating vector operands as scalar values instead of pointers would make
  every 128-bit intrinsic load or store the wrong data.
- Reusing `x0` for destination pointers before saving scalar source values
  would clobber `Movnti`, `Movnti64`, and similar store inputs.
- Emitting byte-lane vector operations for `Pcmpeqd128` would compare the wrong
  lane width; it must use `.4s`.
- A naive `Pmovmskb128` rebuild can easily mix lane order, high-bit extraction,
  or horizontal reduction semantics and produce a byte-reversed or half-swapped
  mask.
- Lowering x86-only AES, CLMUL, shuffle, insert, extract, and packed arithmetic
  intrinsics to silent zero results may hide real unsupported-code paths.
- Assuming `dc civac` is a full x86 `clflush` equivalent would overstate the
  old cache-maintenance contract.
- Using FP arithmetic directly on integer accumulator values without `fmov`
  would reinterpret scalar FP payloads incorrectly.
- Dropping feature-gating for CRC instructions could emit invalid code on
  AArch64 targets without the CRC extension.
- The old F128 comments name compiler-rt/libgcc helper contracts, but this
  file did not implement them; a rebuild should not infer complete binary128
  coverage from this artifact alone.

## Rebuild Guidance

Rebuild this surface around explicit intrinsic families:

Intrinsic lowering should map each accepted intrinsic family to structured
target MIR and machine instruction nodes before printer or encoder handling.
Feature-gated mnemonic choices such as CRC or cache maintenance must be node
identity or encoding-table data, not parser-recovered assembly text.

1. Separate scalar builtins, barriers/cache hints, vector memory operations,
   vector arithmetic, CRC, and unsupported x86-only intrinsic policy.
2. Preserve the pointer-based 128-bit vector operand contract unless a later IR
   carrier replaces it deliberately.
3. Keep raw-bit scalar FP movement through `fmov` visible in the lowering
   contract and tests.
4. Decide whether x86-only intrinsics should remain zero-filled, trap, or be
   rejected earlier before treating AArch64 intrinsic lowering as complete.
5. Gate CRC and cache-maintenance instructions explicitly if the rebuilt
   backend models CPU features.
6. Treat binary128 soft-float lowering as a separate owned surface rather than
   part of this intrinsic extraction.
