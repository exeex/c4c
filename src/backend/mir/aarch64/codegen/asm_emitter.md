# AArch64 Inline Assembly Emitter Legacy Surface

This artifact preserves the useful production shape from the removed
`asm_emitter.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/asm_emitter.rs`.

## Role

The surface implemented `InlineAsmEmitter` behavior for `ArmCodegen`. It covered
constraint classification, scratch register assignment, operand metadata,
input/output movement, template substitution, and AArch64-specific immediate
validation for inline assembly.

The code assumed the generic inline-asm pipeline owned operand discovery,
constraint ordering, tied operand wiring, and label substitution scaffolding.
This AArch64 layer supplied target-specific register names, memory addressing,
stack-slot materialization, and constraint semantics.

## Entry Points

- `asm_state()`: exposes the backend `CodegenState` used by the generic
  inline-asm machinery.
- `classify_constraint(constraint)`: maps GCC-style AArch64 constraints to
  operand kinds, including explicit register names, tied operands, GP registers,
  FP/SIMD registers, memory, immediates, and condition-code outputs.
- `setup_operand_metadata(op, val, is_output)`: records stack-relative memory
  metadata for direct allocas and marks indirect non-alloca slots for later
  address resolution.
- `resolve_memory_operand(op, val, excluded)`: assigns a GP scratch register
  and materializes non-alloca or constant-address memory operands as `[xN]`.
- `assign_scratch_reg(kind, excluded)`: hands out GP or FP/SIMD scratch
  registers while avoiding already excluded names.
- `load_input_to_reg(op, val, constraint)`: loads constants or stack-backed
  values into the operand register before assembly emission.
- `preload_readwrite_output(op, ptr)`: loads the current value of a read-write
  output into its chosen register.
- `substitute_template_line(line, operands, gcc_to_internal, operand_types,
  goto_labels)`: formats operands for template substitution and then resolves
  `%l[...]` goto labels through the generic inline-asm helper.
- `store_output_from_reg(op, ptr, constraint, all_output_regs)`: writes output
  registers back to stack slots or through pointer slots after template
  emission.
- `reset_scratch_state()`: resets GP and FP/SIMD scratch indices per asm block.
- `constant_fits_immediate(constraint, value)`: overrides generic immediate
  validation with AArch64 ranges and logical-immediate rules.

## Helper Surface

- `const_for_asm_operand(c, operand_type)`: converts IR constants to the value
  loaded for an asm operand. Narrow integer sizes are zero-extended, while
  64-bit and larger operands keep signed semantics.
- `normalize_aarch64_register(name)`: maps `r0` through `r30` aliases to
  canonical `x0` through `x30` names for Linux-kernel-style register variables.
- `is_valid_aarch64_logical_immediate_32(value)`: validates 32-bit logical
  immediates for the `L` constraint.
- `is_valid_aarch64_logical_immediate(value)`: validates 64-bit logical
  immediates for the `K` constraint.
- `is_repeating_pattern(value, element, size)` and
  `has_contiguous_ones(element, size)`: implement the shared bitmask-pattern
  test used by logical-immediate validation.

## Constraint Behavior

The classifier stripped common GCC output and modifier prefixes before
interpreting constraints. It preferred registers over memory and immediates for
multi-alternative strings.

- `{reg}` explicit register constraints were treated as specific registers, with
  AArch64 `rN` aliases normalized to `xN`.
- Numeric constraints were treated as tied operands.
- `r` selected GP registers.
- `w` selected FP/SIMD registers.
- `m`, `Q`, `o`, `V`, and `p` selected memory.
- `i`, `n`, `I`, `K`, and `L` selected immediates.
- `g` selected the broad GP/memory/immediate family but still classified as GP
  because registers were preferred.
- `=@cc...` was recognized as a condition-code output, but the comments marked
  it incomplete because result capture would need `CSET`/`CSINC`-style lowering
  in output storage.

Immediate validation accepted unconstrained `i` and `n` constants, `I` values in
the unsigned 12-bit add/sub range, `K` values that fit AArch64 64-bit logical
immediate encoding, and `L` values that fit the 32-bit logical-immediate form.
The logical-immediate checks rejected zero and all-ones values.

## Register And Scratch Policy

The GP scratch pool started with `x9` through `x15`, then included `x19`
through `x21`. The FP/SIMD scratch pool used `v16` through `v25`, preserving the
`v` prefix so unmodified vector templates such as `%0.16b` formatted naturally.

GP scratch assignment advanced a monotonically increasing index and skipped
excluded registers. If it handed out callee-saved `x19` through `x28`, it marked
the physical register as used so the broader prologue/epilogue path would save
and restore it. FP/SIMD scratch assignment had a bounded retry loop and then a
fallback that could return an excluded register if every candidate was
exhausted.

The implementation used fixed scratch `x9` in several data-movement paths,
especially FP bit loading, SP movement, and scalar FP store/load conversion.
That makes conflict avoidance dependent on the broader inline-asm scheduling
and output-storage order.

## Operand Movement

Constants bound to GP registers were loaded with the target immediate loader
after applying `const_for_asm_operand`. Constants bound to FP/SIMD registers
were converted to their bit representation, loaded through `x9`, then moved
with `fmov` into the scalar `sN` or `dN` view. Constants targeting `sp` were
loaded into `x9` first and then moved into `sp`.

Value operands used their `CodegenState` stack slot:

- FP/SIMD 128-bit values loaded or stored directly through the `qN` view.
- 32-bit scalar FP values moved through `w9` and the `sN` view.
- 64-bit scalar FP values moved through `x9` and the `dN` view.
- `sp` inputs and outputs moved through `x9` because AArch64 cannot use `sp` as
  a normal source or destination for several load/store forms.
- Alloca values represented addresses, so the input loader computed the alloca
  address instead of loading slot contents.
- Non-alloca value inputs loaded from their stack slot.
- Direct-slot outputs stored back to their stack slot.
- Pointer-slot outputs loaded the pointer from the stack and stored through it.

For pointer-slot output stores, the old code chose a scratch register from
`x9` through `x15` that did not collide with any output register when possible.
The fallback still risked collision when the output set exhausted the candidate
pool.

## Memory Operand Conventions

Memory operands carried either a preformatted address string, a stack offset, or
a fallback register wrapper.

- Direct allocas used stack-relative `[sp, #offset]` addressing.
- Non-alloca slots were treated as pointer slots and resolved later by loading
  the pointer into a GP scratch register.
- Constant addresses were loaded into a scratch register and formatted as
  indirect `[xN]` memory.
- A completely unresolved memory operand fell back to `[sp]` or `[reg]`.

This split matters for rebuild work because inline asm memory constraints need
to distinguish "the IR value is the memory object" from "the IR value is a
pointer stored in a stack slot."

## Dependencies

The removed surface depended on these surrounding concepts:

- Generic IR entities: `BlockId`, `IrConst`, `Operand`, `Value`, and `IrType`.
- Backend state: stack-slot lookup, alloca detection, direct-slot detection,
  assembly emission, and used callee-saved register tracking.
- Generic inline-asm types and helpers: `InlineAsmEmitter`, `AsmOperandKind`,
  `AsmOperand`, operand substitution, and goto-label substitution.
- Register-allocation model: `PhysReg` for callee-saved bookkeeping.
- AArch64 codegen helpers: `ArmCodegen`, `is_arm_fp_reg`, `emit_load_imm64`,
  `emit_load_from_sp`, `emit_store_to_sp`, `emit_alloca_addr`, and FP register
  view conversions (`s`, `d`, and `q` forms).

## Hidden Assumptions

- `IrType::size()` is available and is the source of truth for choosing
  zero-extension, scalar FP width, and vector-width loads/stores.
- Stack slots use byte offsets suitable for direct textual `[sp, #offset]`
  formatting.
- `x9` is always safe as a transient bridge in paths that do not explicitly ask
  the scratch allocator for conflict-free registers.
- FP/SIMD registers can be represented internally with a `vN` base name and
  converted at the final load/store or `fmov` site to `sN`, `dN`, or `qN`.
- The generic inline-asm pipeline has already mapped GCC operand indices to
  internal operand indices before template substitution.
- Condition-code outputs can be classified before their actual result capture
  is implemented.
- Callee-saved scratch selection is repaired by marking `used_callee_saved`;
  there is no local save/restore in this surface.

## Known Failure Risks For Rebuild

- Condition-code outputs were explicitly incomplete and should not be recreated
  as a false-positive supported path without semantic capture.
- Fixed `x9` bridge use can clobber an operand when the surrounding allocator or
  template also expects `x9` to remain live.
- The FP scratch fallback could return an excluded register after 32 attempts.
- Memory operand resolution relies on stack-slot metadata conventions that must
  be rebuilt deliberately; treating all values as direct stack memory would
  break pointer operands and MMIO-style constant addresses.
- The `g` constraint loses the full alternative set by collapsing to GP because
  of preference ordering.
- Logical-immediate validation is subtle; partial rewrites are likely to accept
  invalid bitmask patterns or reject rotated valid ones.
- SP handling requires special movement through a scratch register for both
  loads and stores.
- Callee-saved GP scratch use requires coordination with function prologue and
  epilogue generation.
- Output stores through pointer slots must avoid clobbering any still-live
  output register, not just the current output.

## Rebuild Guidance

Rebuild this behavior from semantic groups rather than line-for-line:

This surface should become a structured inline-assembly or printer-consumer
boundary, not a direct semantic emitter. Constraint resolution, scratch
lifetimes, memory operands, and condition-code outputs should be represented in
target MIR and machine instruction nodes before any final assembly template is
printed.

1. Establish target constraint classification and immediate validation.
2. Rebuild scratch allocation with explicit lifetime and clobber rules.
3. Rebuild memory operand resolution around direct alloca memory, pointer-slot
   indirection, and constant addresses.
4. Rebuild input preload and output store paths with SP and FP/SIMD register
   view handling as first-class cases.
5. Add condition-code output lowering only when the store path captures the
   condition result into the destination value.
