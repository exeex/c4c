# `scalar_lowering.cpp` extraction

## Purpose and current responsibility

This file is the scalar integer lowering pocket for x86 MIR/BIR codegen. It does not define a new subsystem boundary; it is a behavioral implementation layer attached directly to `X86Codegen`. Its job is to turn integer unary/binary BIR operations into concrete x86 instruction strings while cooperating with the current register-assignment state, accumulator conventions, and store-back helpers.

In practice it owns three things at once:

- instruction selection for integer scalar ops
- local register-routing heuristics for direct-destination emission
- accumulator-based fallback emission when direct lowering is not viable

That mix is why it behaves as both a lowering rule table and a micro-scheduler.

## Contract surfaces

The public surface is still implicit on `X86Codegen`; this file fills in those methods rather than introducing a separate type.

```cpp
namespace c4c::backend::x86 {
// Canonical x86 scalar-lowering seam. The owning method declarations remain on
// X86Codegen in the transitional shared surface for now.
}
```

Implemented entrypoints:

```cpp
void X86Codegen::emit_int_neg_impl(IrType ty);
void X86Codegen::emit_int_not_impl(IrType ty);
void X86Codegen::emit_int_clz_impl(IrType ty);
void X86Codegen::emit_int_ctz_impl(IrType ty);
void X86Codegen::emit_int_bswap_impl(IrType ty);
void X86Codegen::emit_int_popcount_impl(IrType ty);
void X86Codegen::emit_int_binop_impl(const Value& dest,
                                     unsigned op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty);
```

Important local helpers define the real contract pressure:

```cpp
std::optional<PhysReg> assigned_reg(const X86CodegenState&, std::uint32_t value_id);
std::optional<PhysReg> operand_reg(const X86Codegen&, const Operand&);
std::optional<PhysReg> dest_reg(const X86Codegen&, const Value&);
std::optional<std::uint8_t> lea_scale_for_mul(std::int64_t imm);
```

These helpers reveal that the implementation depends on pre-existing register assignment and opportunistically changes strategy when a destination already has a physical register.

## Dependency direction and hidden inputs

Direct dependencies flow inward from shared x86 codegen infrastructure:

- `X86Codegen` methods such as `operand_to_reg`, `operand_to_rax`, `operand_to_rcx`, `store_rax_to`, `const_as_imm32`
- `X86CodegenState` for `emit(...)` and `assigned_reg_index(...)`
- register-name helpers like `phys_reg_name(...)` and `phys_reg_name_32(...)`
- `reg_cache.invalidate_acc()` as hidden cache-coherency maintenance
- BIR enums for opcode identity

Hidden inputs that materially affect behavior:

- whether a value already has an assigned physical register
- whether an operand can be materialized as an `imm32`
- whether the type is treated as 32-bit vs 64-bit
- whether the type is signed vs unsigned, which changes sign-extension repair after 32-bit ops
- x86 architectural conventions around `%rax`, `%rcx`, and `%rdx` for multiply/divide/shift paths

This means the file is not just lowering from IR shape; it is also reading backend state, ABI-ish register conventions, and cache invariants.

## Responsibility buckets

### 1. Type classification

Small helpers classify:

- 32-bit integer mode: `I32`, `U32`
- unsigned mode: `U16`, `U32`, `U64`, `Ptr`

That classification drives opcode suffix choice, shift masking width, and whether 32-bit results need explicit sign-extension back to 64-bit.

### 2. Operand and destination routing

The file decides whether it can write directly into the destination register or must fall back to accumulator-driven emission. It also handles conflict avoidance when a source operand is already in the would-be destination register.

### 3. Immediate-specialized lowering

There are dedicated immediate paths for:

- add/sub/and/or/xor
- multiply, including `lea` for selected constants
- shifts with masked immediate counts

These are performance-oriented but also encode compatibility details about what the surrounding backend expects the final register width to be.

### 4. Accumulator fallback

When direct-destination emission is unavailable, the file normalizes around `%rax` and `%rcx`, with `%rdx` participating in div/rem paths. This fallback is the true semantic baseline for the file.

### 5. Simple unary integer ops

`neg`, `not`, `clz`, `ctz`, `bswap`, and `popcount` are emitted directly against accumulator registers. The file assumes the operand is already resident in `%rax`/`%eax` when these helpers run.

## Notable fast paths

### Direct-destination ALU path

If `dest` already has a physical register, many binary ops bypass `%rax` store-back entirely and emit directly into that register. This is the main locality/perf path.

### Immediate multiply via `lea`

The file recognizes a narrow constant family:

```cpp
std::optional<std::uint8_t> lea_scale_for_mul(std::int64_t imm);
// 3 -> 2, 5 -> 4, 9 -> 8
```

That is a hand-optimized fast path for `x * 3`, `x * 5`, and `x * 9`.

### Zero-immediate load avoidance

When moving an immediate zero into a target callee register, it emits `xor reg, reg` instead of a general move.

## Compatibility paths

### Signed 32-bit repair

After several 32-bit operations, signed results are explicitly widened back to 64-bit using `movslq` or `cltq`. This is not optional cleanup; it preserves the backend's current expectation that signed 32-bit scalar values occupy a correct 64-bit register form afterward.

### Shift-count masking

Immediate shifts are masked to `31` or `63` depending on mode before emission. That mirrors x86 execution semantics and keeps the textual output deterministic.

### Div/rem architectural setup

Division and remainder use the fixed x86 accumulator protocol:

- `%rax` holds the dividend/result base
- `%rcx` holds the divisor
- `%rdx` is prepared for signed or unsigned division and carries remainder output

This is a compatibility dependency on the ISA shape, not a reusable abstraction.

## Overfit pressure and route risk

The file already contains several signs of tactical growth pressure:

- direct-register and accumulator paths duplicate parts of the same opcode matrix
- 32-bit signed-repair rules are repeated across multiple branches
- multiply-by-constant optimization is encoded as a tiny named constant family rather than a general strength-reduction layer
- conflict handling is intertwined with instruction selection instead of being isolated as move-planning

None of these are obviously testcase-shaped today, but this file is structurally vulnerable to that failure mode. Future patches can easily add more one-off constant patterns, more special register conflict branches, or more per-opcode sign-extension fixes.

The rebuild should treat the current `lea` multiply cases and accumulator/direct split as evidence of pressure points, not as the design to preserve verbatim.

## What this file should and should not own in a rebuild

Should own:

- scalar integer lowering semantics for x86
- explicit contracts for operand forms, result width repair, and required machine registers
- a clean separation between semantic lowering and move/register planning

Should not own:

- ad hoc register-conflict choreography mixed into every opcode path
- duplicated direct vs accumulator opcode tables
- scattered signed-width repair policy
- growth by one-off constant or testcase-shaped fast paths without a general rule
