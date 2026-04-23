# `i128_ops.cpp`

## Purpose And Current Responsibility

This file is the x86 backend's concrete emitter for 128-bit integer operations once operands have already been routed into the backend's fixed register-pair convention.

Its job is narrow but mixed:

- emit inline two-lane integer ALU sequences over `%rax`/`%rdx` and `%rcx`/`%rsi`
- bridge some 128-bit operations to compiler-rt style helper calls
- move 128-bit values between accumulator pairs, stack slots, and indirect memory
- materialize boolean compare results back into the normal scalar accumulator/result path

It does **not** decide IR lowering strategy, register allocation policy, or type legality. It assumes those choices were already made upstream.

## Important APIs And Contract Surfaces

Declared on `X86Codegen` as a family of `emit_i128_*` and pair-move helpers:

```cpp
void emit_i128_prep_binop_impl(const Operand& lhs, const Operand& rhs);
void emit_i128_add_impl();
void emit_i128_mul_impl();
void emit_i128_divrem_call_impl(const std::string& func_name,
                                const Operand& lhs,
                                const Operand& rhs);
void emit_i128_to_float_call_impl(const Operand& src, bool from_signed, IrType to_ty);
void emit_float_to_i128_call_impl(const Operand& src, bool to_signed, IrType from_ty);
void emit_i128_cmp_ordered_impl(IrCmpOp op);
void emit_store_pair_to_slot_impl(StackSlot slot);
void emit_load_pair_indirect_impl();
```

Operational contract shared by nearly every function:

```cpp
// lhs/result pair
%rax = low 64 bits
%rdx = high 64 bits

// rhs pair for binops
%rcx = low 64 bits
%rsi = high 64 bits
```

The file depends on external helpers to establish and consume that convention:

```cpp
void operand_to_rax_rdx(const Operand& op);
void prep_i128_binop(const Operand& lhs, const Operand& rhs);
void store_rax_rdx_to(const Value& dest);
```

## Dependency Direction And Hidden Inputs

Dependency direction is inward toward shared backend state:

- `x86_codegen.hpp` defines the public member surface and IR enums.
- `core/x86_codegen_output.cpp` owns operand loading, stack-slot lookup, accumulator caching, and paired-result storage.
- `this->state.emit(...)` is the real output sink; this file mainly feeds it assembly strings.
- `this->state.reg_cache` is a hidden side channel whose validity can be destroyed by helper calls and some moves.

Hidden inputs that shape behavior:

- the backend assumes `%rax/%rdx` are the canonical i128 accumulator pair
- `prep_i128_binop` saves/restores the lhs while loading rhs into `%rcx/%rsi`
- `operand_to_rax_rdx` sign-extends immediate inputs into `%rdx`; this file inherits that interpretation
- helper-call entry/return ABI is assumed rather than declared here
- compare lowering assumes callers exclude equality ops from `emit_i128_cmp_ordered_impl`

## Responsibility Buckets

### 1. Core Inline Lowering

- add/sub use `addq` + carry / `subq` + borrow across the pair
- bitwise ops apply lane-wise to low and high halves
- unary neg/not are implemented directly over the pair
- compare-equality reduces both lanes to a single byte result in `%al`

### 2. Shift Lowering

- variable shifts use `shldq` / `shrdq` plus a `testb $64, %cl` branch to handle cross-lane rollover
- constant shifts mask the amount with `127`
- constant special cases explicitly branch on `0`, `64`, and `>64`

This is a real semantic seam: the file owns the split-into-halves implementation detail for 128-bit shifts.

### 3. Helper-Call Compatibility Paths

- division/remainder are not emitted inline; callers provide the helper symbol name
- i128 <-> float conversions dispatch to libgcc/compiler-rt style helpers such as `__floattidf`, `__floatuntisf`, `__fixdfti`, `__fixunssfti`
- after helper calls the register cache is invalidated and results are moved back into the integer accumulator convention if needed

These paths are compatibility-oriented, not native x86 lowering.

### 4. Pair Transport / Scratch Conventions

- load/store pair from a `Value`
- spill/load pair to `StackSlot`
- snapshot pair into `%rsi/%rdi`
- store/load pair through pointer in `%rcx`
- extend or clear the high lane with `cqto` / `xorl %edx, %edx`

This makes the file partly an i128 data-movement utility, not only an arithmetic emitter.

## Notable Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast Paths

- zero shift returns immediately for constant-shift entry points
- exact `64` and `>64` constant-shift cases avoid generic multi-instruction fallback logic
- equality comparison avoids branching except for the final `setcc`
- basic arithmetic and bitwise ops stay inline and avoid helper calls

### Compatibility Paths

- multiply is still an inline hand-rolled partial-product sequence using fixed scratch registers rather than a generalized wide-multiply abstraction
- div/rem and float conversions defer to external runtime helpers through `@PLT`
- compare ordering is encoded as explicit `IrCmpOp` to `setcc` selection, with invalid modes rejected via `std::invalid_argument`

### Overfit Pressure

- register choices are hard-coded throughout; reuse outside the current accumulator protocol would require duplication or wrapper glue
- variable shifts depend on `%cl` and explicit `64` tests, which couples semantics tightly to current calling/setup rules
- helper selection for conversions is a closed list over `{F32,F64} x {signed,unsigned}`; extending types or ABIs here would encourage more local branching
- stack and indirect pair moves expose backend storage details directly inside the i128 operation module, increasing pressure to keep unrelated transport code here

## Rebuild Ownership

In a rebuild, this area should own only the semantic lowering of i128 operations into either inline x86 pair-lowering or explicit runtime-helper boundaries.

It should **not** own generic pair transport, stack-slot plumbing, cache-management policy, or ad hoc ABI/register choreography that belongs in shared accumulator/operand movement support.
