# `asm_emitter.cpp` extraction

## Purpose and current responsibility

This file is the inline-assembly operand adaptation layer for the x86 MIR backend. It does not define the whole backend interface; it translates GCC-style asm constraints and backend-local operand metadata into concrete x86 register, memory, x87, XMM, condition-code, and immediate moves that the rest of `X86Codegen` can splice into emitted asm text.

Its practical job is:

- classify each asm operand constraint into an internal bucket
- attach operand metadata derived from IR values and stack-slot state
- allocate scratch registers when an operand needs materialization
- move inputs into the expected machine location before the asm template
- preload read/write outputs
- substitute template placeholders with finalized operand spellings
- store outputs back to backend values after the asm block

This is behavior-preserving glue around a legacy surface. It is evidence about how inline asm currently works, not a clean target design.

## Important APIs and contract surfaces

The externally visible surface is a small method family on `X86Codegen`:

```cpp
AsmOperandKind classify_constraint(const std::string& constraint);
void setup_operand_metadata(AsmOperand& op, const Operand& val, bool is_output);
bool resolve_memory_operand(AsmOperand& op, const Operand& val,
                            const std::vector<std::string>& excluded);
std::string assign_scratch_reg(const AsmOperandKind& kind,
                               const std::vector<std::string>& excluded);
void load_input_to_reg(const AsmOperand& op, const Operand& val,
                       const std::string& constraint);
void preload_readwrite_output(const AsmOperand& op, const Value& ptr);
std::string substitute_template_line(const std::string& line,
                                     const std::vector<AsmOperand>& operands,
                                     const std::vector<std::size_t>& gcc_to_internal,
                                     const std::vector<IrType>& operand_types,
                                     const std::vector<std::pair<std::string, BlockId>>& goto_labels);
void store_output_from_reg(const AsmOperand& op, const Value& ptr,
                           const std::string& constraint,
                           const std::vector<const char*>& all_output_regs);
void reset_scratch_state();
```

The local operand record is the key hidden contract:

```cpp
struct AsmOperand {
  AsmOperandKind kind;
  std::string reg;
  std::optional<std::string> name;
  std::string mem_addr;
  std::optional<std::int64_t> imm_value;
  std::optional<std::string> imm_symbol;
  IrType operand_type;
  std::string constraint;
  std::string seg_prefix;
};
```

Interpretation notes:

- `kind` drives the whole load/store path.
- `reg` is both allocation result and later substitution input.
- `mem_addr` may be precomputed or lazily materialized.
- `imm_value` and `imm_symbol` let the template layer choose literal vs symbol spelling.
- `operand_type` is reused in move opcode selection and tied-operand type propagation.
- `seg_prefix` is applied late, during textual substitution, not during address construction.

## Dependency direction and hidden inputs

The explicit include surface is only `x86_codegen.hpp`, but the real dependency direction points inward to broad backend state:

- `X86CodegenState::slots`, `allocas`, and over-alignment metadata decide whether an operand is direct stack memory, pointer-valued memory, or requires address materialization.
- `X86CodegenState::out` and `emit(...)` are the actual side-effect sinks.
- register-width helpers on `X86Codegen` (`reg_to_8l`, `reg_to_16`, `reg_to_32`, `reg_to_64`) determine textual register spelling.
- move-selection helpers (`mov_load_for_type`, `mov_store_for_type`) and condition-code mapping (`gcc_cc_to_x86`) are hidden policy inputs.
- `substitute_x86_asm_operands(...)` is the downstream template engine; this file assembles arguments for it but does not own the substitution rules.

One especially non-obvious hidden input is the static scratch allocator state keyed by `const X86Codegen*`. Scratch allocation is therefore stateful across calls until `reset_scratch_state()` runs.

## Responsibility buckets

### 1. Constraint decoding

`classify_constraint` strips GCC prefix markers (`=`, `+`, `&`, `%`) and collapses many constraint spellings into backend-local buckets:

- specific register constraints like `{rax}`
- tied outputs via digit-only references
- condition-code outputs via `@cc...`
- x87 stack positions `t` and `u`
- GP, Q-register, FP/XMM, memory, and immediate classes

This is a compatibility parser, not a principled asm type system.

### 2. Operand metadata shaping

`setup_operand_metadata` and `resolve_memory_operand` combine the requested kind with IR value facts:

- direct stack-slot references for plain stack-resident values
- computed addresses for `alloca` values, including over-alignment repair
- RIP-relative symbol memory for symbolic immediates
- temporary-register materialization for pointer-valued slots or literal addresses

The return value of `resolve_memory_operand` is a side-effect hint: `true` means extra setup instructions were emitted.

### 3. Scratch register allocation

`assign_scratch_reg` hands out:

- GP scratch from a fixed preferred list, then synthetic `r12`..`r15` progression
- XMM scratch from `xmm0`..`xmm15`
- Q-register compatibility scratch from legacy `rax`/`rbx`/`rcx`/`rdx`

Exclusion is best-effort. Exhaustion falls back to fixed defaults such as `rcx` or `xmm0`, so this is not a full allocator and can hide collision risk.

### 4. Input staging

`load_input_to_reg` materializes operands into the machine location implied by `AsmOperand`:

- x87 inputs are loaded with `fld*`
- XMM inputs use `movss` / `movsd` / `movdqu`
- integer immediates prefer zeroing xor, 32-bit `mov`, or `movabsq` depending on range
- `alloca` values are treated as addresses and loaded with `lea`
- non-alloca stack slots are loaded by type-aware move helpers

### 5. Read/write output preload

`preload_readwrite_output` mirrors the input path for `+`-style outputs so the asm body sees the current memory value in the destination register or x87 stack slot before execution.

### 6. Template substitution preparation

`substitute_template_line` does not parse asm semantics. It gathers flat substitution vectors:

- chosen registers
- operand names
- memory-vs-register flags
- finalized memory addresses, including late segment prefixes
- immediate values and symbols
- operand types, including propagation from tied operands

It then delegates to the shared x86 substitution helper.

### 7. Output write-back

`store_output_from_reg` stores post-asm results back to backend values:

- x87 outputs use `fstp*`
- condition-code outputs synthesize `setcc` plus zero-extension before store
- XMM outputs use the same size-selected XMM move family
- integer outputs use typed store helpers
- pointer-valued destinations require an extra scratch register and push/pop save around indirect stores

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- Immediate zero becomes xor-zeroing instead of a literal move.
- Small immediates use narrower moves before falling back to `movabsq`.
- Direct stack-slot addressing avoids scratch setup when the value shape allows it.
- XMM and x87 cases bypass generic integer move helpers.

### Compatibility paths

- GCC constraint parsing accepts many legacy letters and folds them into a smaller internal enum.
- x87 `t` / `u` support preserves old stack-based FP behavior.
- Condition-code outputs are encoded through the GCC `@cc...` spelling.
- Over-aligned `alloca` handling is repaired locally by `lea` + add + and masking.
- Non-alloca pointer destinations are handled through temporary register indirection and explicit save/restore.

### Overfit pressure

This file already mixes several policy layers that want to separate in a rebuild:

- constraint-language parsing
- register-choice heuristics
- stack/address-shape recovery from backend state
- textual asm rendering helpers
- x87 legacy compatibility
- output write-back conventions

Pressure points that look especially brittle:

- static scratch state keyed by object identity instead of packet-local allocation context
- ad hoc fallback registers on exhaustion
- duplicated special handling for `alloca` vs non-`alloca`
- condition-code and x87 support living beside normal GP/XMM flows instead of behind distinct strategy seams
- textual substitution preparation coupled to semantic operand preparation

## Rebuild ownership guidance

In a rebuild, this unit should own only inline-asm operand adaptation: constraint classification, operand-shape normalization, and handoff into a narrower render/store pipeline.

It should not own general scratch allocation policy, broad stack-slot interpretation, backend-wide move opcode policy, or the final text-substitution engine for all x86 asm rendering.
