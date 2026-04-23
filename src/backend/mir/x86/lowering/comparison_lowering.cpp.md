# `comparison_lowering.cpp`

## Purpose and Current Responsibility

This file lowers MIR comparison-shaped operations into x86 flag-producing instruction sequences and consumes those flags in three main ways:

- materialize boolean compare results into a destination value
- fuse integer compare + branch emission
- lower generic condition-based branching and `select`

It owns both integer and floating-point comparison lowering, including a separate `F128` path through x87 state. It is not just a compare helper: it also decides operand staging, instruction width, branch opcode selection, and the boolean materialization convention (`%al` -> zero-extended to `%rax` -> stored).

## Important APIs and Contract Surfaces

The visible ownership is a set of `X86Codegen` member implementations:

```cpp
void X86Codegen::emit_float_cmp_impl(const Value& dest,
                                     IrCmpOp op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty);

void X86Codegen::emit_int_cmp_impl(const Value& dest,
                                   IrCmpOp op,
                                   const Operand& lhs,
                                   const Operand& rhs,
                                   IrType ty);
```

```cpp
void X86Codegen::emit_fused_cmp_branch_impl(IrCmpOp op,
                                            const Operand& lhs,
                                            const Operand& rhs,
                                            IrType ty,
                                            const std::string& true_label,
                                            const std::string& false_label);

void X86Codegen::emit_select_impl(const Value& dest,
                                  const Operand& cond,
                                  const Operand& true_val,
                                  const Operand& false_val,
                                  IrType ty);
```

Internal contract helpers are small but decisive:

```cpp
const char* cmp_setcc(IrCmpOp op);
const char* cmp_jcc(IrCmpOp op);
void emit_int_cmp_insn_typed(X86Codegen&, const Operand&, const Operand&, bool use_32bit);
void emit_float_cmp_result(X86CodegenState&, IrCmpOp op);
```

These helpers define the file’s real surface:

- `IrCmpOp` must map cleanly to both `setcc` and `jcc`
- integer compare emission assumes operands will be staged into `%rax` and `%rcx`
- float compare result materialization assumes flags already reflect `ucomis*`/`fucomip`
- boolean results are normalized as zero/nonzero integer values in `%rax`

## Dependency Direction and Hidden Inputs

The file points inward to `X86Codegen` infrastructure rather than standing alone. Hidden inputs include:

- register conventions baked into helper calls: `%rax`, `%rcx`, `%rdx`, `%xmm0`, `%xmm1`, `%al`, `%cl`
- `operand_to_*` helpers deciding how immediates, memory, and cached values reach those registers
- `store_rax_to(dest)` as the canonical “boolean result commit” mechanism
- `state.emit(...)` and `state.out.emit_instr_imm_reg(...)` as the assembler text boundary
- `state.reg_cache.invalidate_acc()` and `invalidate_all()` as required cache coherence after flag/register side effects
- `emit_f128_load_to_x87(...)` for the 128-bit float compatibility path

This means the file’s correctness depends on calling conventions from surrounding codegen state, not just on local opcode selection.

## Responsibility Buckets

### 1. Compare opcode selection

- maps `IrCmpOp` to integer `setcc` and branch `jcc`
- treats signed and unsigned integer predicates distinctly
- leaves float predicate interpretation to later flag consumption logic rather than a separate float-opcode map

### 2. Integer compare lowering

- decides 32-bit vs 64-bit compare width from `IrType`
- stages operands into `%rax` and `%rcx`
- emits `cmpl` or `cmpq`
- consumes flags via `setcc`

### 3. Float compare lowering

- uses SSE compare setup for `F32`/`F64`
- uses x87 stack-based compare for `F128`
- swaps operand order for less-than / less-or-equal style ops so one flag-decoding path can serve multiple predicates
- converts flag state into a boolean byte result

### 4. Branch lowering

- emits compare + conditional jump + explicit false jump for fused integer compare branches
- supports both string labels and `BlockId` labels
- lowers generic condition branches with `testq` against `%rax`

### 5. Select lowering

- evaluates false value into `%rax` and true value into `%rcx`
- normalizes the condition into `%rdx`
- uses `cmovne` as a branchless conditional move

## Notable Fast Paths, Compatibility Paths, and Overfit Pressure

### Optional fast paths

- `emit_fused_cmp_branch_impl` and `emit_fused_cmp_branch_blocks_impl` avoid materializing a boolean compare result before branching
- `emit_select_impl` uses `cmovne` instead of a branchy select sequence
- immediate conditions in `emit_select_cond_to_rdx` avoid generic operand loading, with dedicated zero, 32-bit-immediate, and `movabsq` cases

### Legacy compatibility

- `F128` lowering goes through x87 (`fucomip` + `fstp`) instead of SSE, so this file bridges two floating-point execution models
- float equality/inequality explicitly handles unordered cases with multi-instruction flag logic (`setnp`/`sete`, `setp`/`setne`)
- the file relies on old fixed-register conventions and explicit cache invalidation rather than a more declarative value/flag model

### Overfit pressure

- the operand-swapping logic for float predicates is compact but implicit; it compresses multiple semantic cases into one flag interpretation path, which is easy to preserve incorrectly during rebuild
- register choices are hardcoded per lowering shape, so the file mixes semantic lowering with tactical register choreography
- `emit_select_impl` ignores `ty`, signaling that current ownership is biased toward one integer-like boolean/select representation rather than a type-directed select abstraction
- the default fallbacks in `cmp_setcc` and `cmp_jcc` return equality opcodes, which is survivable but weak as a contract boundary if enum coverage ever drifts

## Rebuild Ownership Boundary

This file should own compare/condition lowering semantics: predicate interpretation, result-shape decisions, and the distinction between materialized booleans, fused branches, and selects.

This file should not own long-term register choreography policy, x87 compatibility plumbing, or low-level flag-to-register encoding details that could live behind smaller target-specific helpers.
