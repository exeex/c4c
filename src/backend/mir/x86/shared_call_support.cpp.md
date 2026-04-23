# `shared_call_support.cpp` extraction

## Purpose and current responsibility

This file is a small shared-support implementation unit for x86 MIR codegen. Despite the name, it is not primarily call lowering logic. Its real job is to provide common state-manipulation and operand-materialization helpers that other emit paths rely on:

- bind `X86CodegenState` to its output sink after copy/move
- append formatted assembly text into the current function body
- answer basic stack-slot and register-assignment queries from `X86CodegenState`
- materialize simple operands into fixed registers, especially `%rax`
- store `%rax` or `%rax/%rdx` back to stack slots
- expose compatibility shims for regalloc / stack-space helpers that are currently stubbed or forwarded elsewhere

The file acts as glue between header-declared shared state and the wider emitter. It is evidence of what data the backend expects to be globally reachable while emitting instructions.

## Important APIs and contract surfaces

Primary state/output surface:

```cpp
struct X86CodegenOutput {
  X86CodegenState* owner = nullptr;
  void bind(X86CodegenState* owner_state);
  void emit_instr_imm_reg(const char* mnemonic, std::int64_t imm, const char* reg);
  void emit_instr_reg_rbp(const char* mnemonic, const char* reg, std::int64_t offset);
  void emit_instr_rbp_reg(const char* mnemonic, std::int64_t offset, const char* reg);
  void emit_instr_rbp(const char* mnemonic, std::int64_t offset);
  void emit_instr_mem_reg(const char* mnemonic,
                          std::int64_t offset,
                          const char* base_reg,
                          const char* dest_reg);
};
```

This is a text-emission facade over `X86CodegenState::asm_lines`. The hidden contract is that `owner` must point back to the currently active state object; the copy/move constructors and assignments in this file exist mostly to preserve that invariant.

State/query surface implemented here:

```cpp
struct X86CodegenState {
  X86CodegenOutput out;
  X86CodegenRegCache reg_cache;
  std::vector<std::string> asm_lines;
  std::unordered_map<std::uint32_t, StackSlot> slots;
  std::unordered_map<std::uint32_t, std::uint8_t> reg_assignment_indices;
  std::unordered_set<std::uint32_t> allocas;
  std::unordered_map<std::uint32_t, std::size_t> over_aligned_allocas;

  void rebind_output();
  void emit(const std::string& asm_line);
  std::optional<StackSlot> get_slot(std::uint32_t value_id) const;
  std::optional<std::uint8_t> assigned_reg_index(std::uint32_t value_id) const;
  std::optional<SlotAddr> resolve_slot_addr(std::uint32_t value_id) const;
  void track_f128_load(std::uint32_t dest_id, std::uint32_t ptr_id, std::int64_t offset);
  std::optional<std::uint32_t> get_f128_source(std::uint32_t value_id) const;
  void set_f128_constant_words(std::uint32_t operand_id, std::uint64_t lo, std::uint64_t hi);
  bool needs_got_for_addr(std::string_view name) const;
};
```

Materialization/storage surface:

```cpp
struct X86Codegen {
  std::optional<std::int64_t> const_as_imm32(const Operand& op) const;
  void operand_to_reg(const Operand& op, const char* reg);
  void operand_to_rax(const Operand& op);
  void store_rax_to(const Value& dest);
  void store_rax_rdx_to(const Value& dest);
};
```

These are shared fallback helpers for "load operand into a working register" and "spill result back to stack". The file only handles simple integer/stack/register cases; wider or typed lowering lives elsewhere.

Compatibility/regalloc bridge surface:

```cpp
void collect_inline_asm_callee_saved_x86(const IrFunction& func,
                                         std::vector<PhysReg>& asm_clobbered_regs);
std::vector<PhysReg> filter_available_regs(const std::vector<PhysReg>& callee_saved,
                                           const std::vector<PhysReg>& asm_clobbered);
std::pair<std::unordered_map<std::string, std::uint8_t>, std::optional<LivenessResult>>
run_regalloc_and_merge_clobbers(...);
std::int64_t calculate_stack_space_common(...);
```

Current behavior is mixed:

- `filter_available_regs` forwards to `stack_layout::filter_available_regs`
- `collect_inline_asm_callee_saved_x86` is a no-op stub
- `run_regalloc_and_merge_clobbers` is a pass-through stub that returns the incoming assignments and no liveness
- `calculate_stack_space_common` returns `initial_space` unchanged

These APIs remain because other translation units still depend on the shared header surface.

## Dependency direction and hidden inputs

Dependency direction is inward toward shared backend state:

- includes `x86_codegen.hpp` for the declared state and helper surface
- includes regalloc and stack-layout helpers, but this file only lightly participates
- writes raw assembly text into `X86CodegenState::asm_lines`
- resolves operands through shared maps keyed by backend value ids

Hidden inputs the file depends on:

- `slots`, `reg_assignment_indices`, `allocas`, and `over_aligned_allocas` must already be populated correctly by earlier frame/regalloc setup
- `phys_reg_name(...)` and `reg_name_to_32(...)` define register spelling and width narrowing
- `emit_alloca_addr_to(...)` handles indirect / over-aligned alloca address formation; this file delegates rather than owning the addressing rule
- callers rely on `reg_cache` invalidation/update side effects after `%rax` loads and stores
- `got_addr_symbols`, `f128_load_sources`, and `f128_constant_words` are side tables whose lifetime is bound to the whole codegen state object, not to local emit routines

## Responsibility buckets

1. State ownership repair
   Copy/move constructors and assignments rebind `out.owner` so a copied `X86CodegenState` still emits into itself instead of the source object.

2. Assembly text appending
   Small formatting helpers build simple AT&T-syntax instructions and append them to `asm_lines`.

3. Value-location queries
   `get_slot`, `assigned_reg_index`, `is_alloca`, and `resolve_slot_addr` classify whether a value is stack-direct, stack-indirect, or over-aligned.

4. Local bookkeeping side tables
   Tracks f128 load provenance, f128 constant words, and GOT-address requirements.

5. Operand materialization
   `operand_to_reg` chooses among immediate, assigned register, stack slot, alloca address, or zero fallback.

6. Result spillback
   `store_rax_to` and `store_rax_rdx_to` write results to stack slots and update/invalidate the accumulator cache.

7. Legacy compatibility bridges
   Regalloc and stack-space entry points survive here mostly to preserve link/header shape while real ownership has drifted elsewhere.

## Notable fast paths, compatibility paths, and overfit pressure

Fast paths:

- zero immediates use `xor` on the narrowed 32-bit register instead of `mov $0, ...`
- register-assigned operands avoid stack loads and emit a move only when the source register differs from the requested destination
- direct stack slots use the output helper directly without going through more elaborate addressing logic

Compatibility paths:

- alloca and over-aligned values collapse into `emit_alloca_addr_to(...)` instead of being distinguished here beyond `SlotAddr::Kind`
- unresolved operands silently fall back to zeroing the destination register
- several regalloc/stack helpers preserve old call sites through stub or pass-through implementations rather than authoritative logic

Overfit pressure:

- the file has become a catch-all "shared support" bucket where unrelated concerns coexist: state copying, asm formatting, frame-slot queries, f128 side tables, operand loading, and regalloc compatibility hooks
- name and contents are misaligned: almost nothing here is specifically about shared call support
- stubbed compatibility functions invite more incidental reuse because the symbol already exists, even when the real logic belongs in narrower regalloc or frame-lowering modules
- the "zero if not found" operand fallback is convenient but can hide missing location-plumbing bugs instead of surfacing them structurally

## Rebuild ownership

This file should own only small, explicit shared primitives that are truly cross-cutting for x86 emission: state binding, basic output append helpers, and minimal operand/slot utilities with stable contracts.

This file should not own regalloc policy, stack-size computation policy, inline-asm clobber analysis, broad call lowering, f128 strategy, or miscellaneous compatibility stubs kept alive only because the shared header currently exports them.
