# `mod.cpp` extraction

## Purpose and current responsibility

This file is a residual x86 backend compatibility bucket. It does not perform full MIR-to-assembly lowering by itself. Instead, it centralizes small target-local policies and string/render helpers that many sibling translation units still consume through `x86_codegen.hpp`.

The responsibilities present here are:

- x86 register-name normalization across width variants
- SysV-style argument register and stack-slot layout helpers
- callee-saved vs caller-saved register policy helpers
- parameter pre-store and aggregate-copy offset math
- variadic register-save-area and frame-layout helpers
- assembler string decoding/escaping and symbol-label prelude rendering
- a thin shared regalloc configuration entry point
- a link anchor that keeps this compatibility bucket alive during the rebuild

The file is already self-classified as glue around rebuilt owners rather than the place where new canonical lowering should accumulate.

## Important APIs and contract surfaces

Most consumers reach this file indirectly through the broad helper declarations in `x86_codegen.hpp`. The notable exported surface is a collection of free functions rather than a scoped helper object, so hidden coupling is high.

Essential surface examples:

```cpp
std::string x86_format_reg(std::string_view reg, std::optional<char> modifier);
const char* x86_gcc_cc_to_x86(std::string_view cond);
std::vector<c4c::backend::PhysReg> x86_callee_saved_regs();
std::vector<c4c::backend::PhysReg> x86_prune_caller_saved_regs(
    bool has_indirect_call, bool has_i128_ops, bool has_atomic_rmw);
```

```cpp
const char* x86_arg_reg_name(std::size_t reg_index);
std::int64_t x86_param_stack_offset(std::int64_t class_stack_offset);
std::string x86_param_slot_name(std::string_view param_name);
bool x86_param_can_prestore_direct_to_reg(
    bool has_stack_slot,
    std::optional<c4c::backend::PhysReg> assigned_reg,
    std::size_t assigned_param_count);
```

```cpp
c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::backend::LivenessInput& liveness_input);
std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name);
std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name);
```

Contract notes:

- Register helpers assume x86-64 integer register families and preserve raw SIMD/x87 names instead of normalizing them.
- `PhysReg.index` is treated as a target-specific encoding. The mappings are hardcoded rather than table-driven.
- ABI helpers assume the six GP arg registers and eight XMM arg registers used by SysV x86-64.
- Several helpers return `""` for unsupported cases instead of an error type, so callers must know when empty means "unsupported" vs "bug".
- `run_shared_x86_regalloc` always seeds regalloc with this file's register policy; consumers do not pass a target profile object.

## Dependency direction and hidden inputs

Nominal dependencies flow inward from public x86 codegen helpers toward backend abstractions:

- `x86_codegen.hpp` exposes the helper surface
- this file depends on backend core types such as `PhysReg`, `LivenessInput`, `RegAllocIntegrationResult`, and BIR opcodes
- regalloc setup delegates to `run_regalloc_and_merge_clobbers`
- symbol rendering depends on `target_triple` string inspection rather than a normalized target object

Hidden inputs and unstated contracts:

- Darwin vs non-Darwin object format behavior is selected with substring checks on `target_triple`
- stack math assumes fixed constants such as parameter stack base `16` and stack probe page size `4096`
- ABI layout helpers encode "no struct split reg+stack" as a hard false policy
- variadic save-area sizes and offsets are hardcoded to the SysV layout
- parameter-slot naming depends on textual conventions like `%p.` and `%lv.param.`
- caller-saved pruning is driven by coarse booleans (`has_indirect_call`, `has_i128_ops`, `has_atomic_rmw`) rather than a richer hazard model

## Responsibility buckets

### Core lowering support

- register-width conversion and operand formatting
- condition-code name mapping
- register-class sets for x86 regalloc
- parameter/register/stack offset math used by lowering code elsewhere
- frame alignment, stack probing threshold, callee-saved spill slot offsets
- assembler escaping and symbol/prelude rendering

### Optional fast paths

- `x86_param_can_prestore_direct_to_reg` allows a narrow "parameter lands directly in a callee-saved register" path when there is no stack slot and only one assigned consumer
- `x86_prune_caller_saved_regs` shrinks the allocator candidate set based on a few known hazards instead of a more explicit machine model

These are small policy shortcuts, not full lowering algorithms.

### Legacy compatibility

- broad free-function helper surface kept in `x86_codegen.hpp`
- hardcoded `PhysReg.index` to register-name mapping
- `x86_codegen_module_anchor()` exists purely so residual linking continues while ownership moves elsewhere
- many helpers return textual assembly fragments or stringly-typed decisions because sibling legacy translation units still expect that style

### Overfit pressure

- repeated hardcoded register-name tables across widths
- target selection by substring matching on `target_triple`
- unsupported cases silently falling back to empty string or passthrough strings
- ABI behavior baked into many tiny helpers, which encourages adding more one-off policy branches instead of consolidating them behind typed target/ABI objects

This file is not testcase-shaped in the narrow sense, but it is structurally prone to policy accretion.

## Notable fast paths, compatibility paths, and overfit pressure

The most important compatibility path is that this file acts as a shared helper dumping ground for sibling emitters and lowering code. That keeps the rebuild linkable, but it also means new responsibilities can be added here without any obvious ownership review.

The most notable fast path is parameter pre-store eligibility into callee-saved registers. It is useful, but its conditions are encoded as local booleans and register-category assumptions instead of a reusable ABI assignment object.

The largest overfit pressure comes from atomized helper growth:

- one function per register-width conversion
- one function per parameter-shape offset pattern
- one function per platform-label spelling rule

That shape makes it easy to patch another special case and hard to see the single authoritative model of x86 calling convention and assembly rendering.

## Rebuild ownership boundary

In a rebuild, this file should own only truly shared, typed x86 target facts that remain broadly reusable across emitters, such as stable register metadata or a small ABI policy surface.

It should not own lowering decisions, textual assembly construction scattered across unrelated concerns, residual compatibility anchors, or another expanding pile of stringly-typed helper functions. Those should move into narrower owners with explicit target/ABI data models and clearer dependency seams.
