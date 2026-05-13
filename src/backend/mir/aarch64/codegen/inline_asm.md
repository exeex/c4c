# AArch64 Inline Assembly Legacy Surface

This artifact preserves the useful production shape from the removed
`inline_asm.cpp` translation surface. The old file was not active C++; it was
a line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/inline_asm.rs`.

## Role

The surface implemented AArch64 inline assembly operand substitution and a
small set of helper routines shared with atomic lowering. It converted GCC-like
inline assembly operand references such as `%0`, `%[name]`, `%w0`, `%x0`, and
`%%` into target assembly text using the backend's allocated operand registers
and immediate operands.

The substitution helper was intentionally static and string-oriented: callers
passed already-assigned operand registers, optional operand names, a
GCC-operand-number to internal-operand-index map, optional immediate values,
and optional immediate symbols. The result was a substituted assembly line.

## Entry Points

- `substitute_asm_operands_static(line, op_regs, op_names, gcc_to_internal,
  op_imm_values, op_imm_symbols)`: rewrote one template line by replacing
  positional and named operand markers.
- `format_operand_static(idx, modifier, op_regs, op_imm_values,
  op_imm_symbols)`: rendered an immediate symbol, immediate integer, or
  register operand.
- `format_reg_static(reg, modifier)`: normalized AArch64 register spelling and
  applied width or class modifiers.
- `fp_to_s_reg(reg)`, `fp_to_d_reg(reg)`, and `fp_to_q_reg(reg)`: converted
  FP/SIMD register names to a specific scalar/vector spelling while preserving
  the register number.
- `exclusive_instrs(ty, ordering)`: selected AArch64 exclusive load/store
  mnemonics and the GP register prefix for atomic operations.
- `emit_atomic_op_arm(state, op, dest_reg, old_reg, val_reg)`: emitted the ALU
  part of an atomic read-modify-write loop.

## Operand Substitution

The template scanner walked the input line as characters and copied ordinary
characters through unchanged. A percent sign started operand parsing:

- `%%` emitted a literal `%`.
- `%0`, `%1`, and other digit forms selected positional operands.
- `%[name]` selected a named operand by matching the name against
  `op_names`.
- `%w0`, `%x0`, `%d0`, `%q[name]`, and similar forms applied a register or
  formatting modifier before rendering the operand.

Positional operands used GCC numbering, where outputs are numbered first and
then real inputs follow. The helper mapped that external number through
`gcc_to_internal` when possible; if the GCC number was outside the map it used
the number directly. If the resolved internal index was outside `op_regs`, the
legacy fallback emitted `x<num>`.

Named operands searched `op_names` linearly. If no matching name existed, the
old surface preserved the original marker text, including the modifier when
present. This kept unknown `%[name]` references visible in the generated text
instead of silently inventing an operand.

Unrecognized percent forms were emitted as-is with their leading `%` and any
parsed modifier.

## Modifiers

The parser recognized a modifier only when one of these letters was followed by
a digit or `[`:

```text
w x h b s d q c a
```

The old behavior for each modifier was:

- `w`: render a GP register as its 32-bit `wN` form.
- `x`: render a GP register as its 64-bit `xN` form.
- `s`: render an FP/SIMD register as `sN`.
- `d`: render an FP/SIMD register as `dN`.
- `q`: render an FP/SIMD register as `qN`.
- `h`: render an FP/SIMD register as `hN`.
- `b`: render an FP/SIMD register as `bN`.
- `c`: accepted by the parser for raw constants but did not have separate
  register behavior; immediate operands were already emitted without a `#`
  prefix.
- `a`: render a memory address operand by wrapping a non-memory register in
  square brackets, such as `x0` to `[x0]`. Existing bracketed memory references
  were returned unchanged.

When no modifier was supplied, FP/SIMD registers were emitted as `vN`, matching
GCC behavior for forms such as `%0.16b`, while GP registers kept their existing
spelling.

## Immediate Formatting

Immediate operands took priority over register rendering:

1. If `op_imm_symbols[idx]` was present, the symbol text was returned exactly.
2. Otherwise, if `op_imm_values[idx]` was present, the integer was converted to
   decimal text.
3. Otherwise, `op_regs[idx]` was formatted as a register.

The legacy behavior deliberately emitted AArch64 immediate values as raw text
with no `#` prefix. That matched GCC inline assembly handling for `i` and `n`
constraints in both instruction and data-directive contexts. The `%c` modifier
was therefore effectively a parse-level compatibility feature rather than a
separate immediate rendering mode.

## Register Normalization

`format_reg_static` normalized GCC AArch64 aliases `r0` through `r30` to
`x0` through `x30` before applying modifiers. Values outside that range or
non-numeric `r` names were left unchanged.

The helper extracted register numbers from `x`, `w`, `d`, `s`, `q`, and `v`
prefixes. FP/SIMD classification was based on `d`, `s`, `q`, or `v`. Width
modifiers only converted within the appropriate register class:

- GP modifiers `w` and `x` converted only GP registers.
- FP/SIMD modifiers `b`, `h`, `s`, `d`, and `q` converted only FP/SIMD
  registers.
- Incompatible modifier/register-class pairs returned the original register.

## Atomic Exclusive Helpers

The old inline-asm surface also carried helper routines used by AArch64 atomic
lowering.

`exclusive_instrs(ty, ordering)` selected the load-exclusive mnemonic,
store-exclusive mnemonic, and GP register prefix for a type:

- `I8` and `U8`: byte exclusives, `ldxrb`/`ldaxrb` and `stxrb`/`stlxrb`, with
  `w` registers.
- `I16` and `U16`: halfword exclusives, `ldxrh`/`ldaxrh` and
  `stxrh`/`stlxrh`, with `w` registers.
- `I32` and `U32`: word exclusives, `ldxr`/`ldaxr` and `stxr`/`stlxr`, with
  `w` registers.
- All other types: doubleword exclusives, `ldxr`/`ldaxr` and
  `stxr`/`stlxr`, with `x` registers.

Acquire ordering selected the acquire load form (`ldaxr*`) for `Acquire`,
`AcqRel`, and `SeqCst`. Release ordering selected the release store form
(`stlxr*`) for `Release`, `AcqRel`, and `SeqCst`. Relaxed ordering used plain
`ldxr*` and `stxr*`.

`emit_atomic_op_arm` emitted the ALU operation inside an atomic RMW loop:

- `Add`: `add dest, old, val`
- `Sub`: `sub dest, old, val`
- `And`: `and dest, old, val`
- `Or`: `orr dest, old, val`
- `Xor`: `eor dest, old, val`
- `Nand`: `and dest, old, val` followed by `mvn dest, dest`
- `Xchg` and `TestAndSet`: `mov dest, val`

## Dependencies

The removed surface depended on these surrounding concepts:

- Operand register assignment and GCC numbering supplied by the inline
  assembly emitter.
- Optional named operands from frontend/lowering metadata.
- Optional immediate integer and symbol operands for immediate-only
  constraints.
- AArch64 register spelling conventions for GP and FP/SIMD classes.
- Rust `AtomicOrdering`, `AtomicRmwOp`, and `IrType` classifications in the
  reference implementation.
- Assembly emission through `CodegenState::emit_fmt` for the atomic ALU helper.

## Hidden Assumptions

- `op_regs[idx]` exists whenever neither an immediate symbol nor an immediate
  value exists for that index.
- Named operand lookup by first matching name is sufficient; duplicate names
  were not handled specially.
- The `gcc_to_internal` map already accounts for output operands, real inputs,
  and synthetic read-write input elision.
- Unknown named operands should remain visible in the final assembly text
  rather than causing a hard error at substitution time.
- AArch64 inline assembly immediate operands should not gain an automatic `#`
  prefix.
- No-modifier FP/SIMD operands should render as `vN`, not the allocator's
  `sN`, `dN`, or `qN` spelling.
- The `%a` modifier assumes that square brackets are enough to convert a
  register operand into the memory-reference spelling needed by prefetch forms.
- Atomic helper instruction selection assumes exclusive-access loops are built
  elsewhere and this surface only supplies mnemonics and the ALU operation.

## Known Failure Risks For Rebuild

- Treating GCC operand numbers as internal indexes directly would break
  templates with outputs or read-write operands.
- Adding `#` to immediate integers would diverge from GCC-style AArch64 inline
  assembly and could corrupt data directive templates.
- Dropping `r0`-`r30` alias normalization would make `%w0` and `%x0` render
  incorrectly for r-prefixed operands.
- Rendering unmodified FP/SIMD operands as `dN` or `sN` would break vector
  suffix templates such as `%0.16b`.
- Applying GP modifiers to FP/SIMD registers, or FP/SIMD modifiers to GP
  registers, would create invalid register names instead of preserving the
  original operand.
- Losing `%a` bracket behavior would regress prefetch-style templates such as
  `prfm pldl1keep, %a0`.
- Collapsing acquire/release atomic ordering into plain exclusive operations
  would lose the memory-ordering semantics encoded by `ldaxr*` and `stlxr*`.
- Rebuilding this around rendered-name lookup alone would conflict with the
  markdown-first reconstruction goal of preserving structured operand facts.

## Rebuild Guidance

Rebuild this surface around explicit inline-assembly operand records:

Inline assembly remains a separately owned external/textual boundary. When it
interacts with backend lowering, operands, clobbers, constraints, and side
effects should be structured target MIR or machine-node facts before any final
template formatting; parser recovery from emitted text must not supply codegen
semantics.
Register constraints, tied operands, clobbers, output homes, and temporary
scratch must consume `../ALLOCATION_CONTRACT.md` and a later inline-asm
contract. Inline asm must not run an independent register allocator or spill
planner.

1. Keep positional and named operand resolution separate, with a first-class
   GCC-number to internal-index map.
2. Preserve immediate symbols and integer immediates as structured facts before
   falling back to register formatting.
3. Model register class and width explicitly so modifiers cannot create invalid
   spellings.
4. Keep unknown operand references observable for diagnostics unless the new
   frontend contract requires an earlier hard error.
5. Prove literal percent handling, positional and named substitution, each
   register modifier, raw immediate formatting, `%a` memory-reference
   formatting, and atomic exclusive mnemonic selection independently.
