# `x86_codegen_output.cpp` extraction

## Purpose and current responsibility

This file is a mixed "output/runtime glue" unit for the legacy x86 MIR backend. Despite the basename, it does not just format output text. It currently owns four different layers:

1. small register-name and ABI lookup helpers
2. `X86CodegenOutput` line-emission helpers that append textual asm into `X86CodegenState::asm_lines`
3. `X86CodegenState` storage/query/update helpers for slots, cached register state, f128 metadata, and GOT-address flags
4. `X86Codegen` helper routines for loading operands, storing results, return-value shuffles, type-width instruction selection, and a generic atomic RMW loop emitter

The file therefore acts as a grab-bag of backend plumbing rather than a clean output-only module.

## Important APIs and contract surfaces

The real contract surface is declared in `src/backend/mir/x86/x86_codegen.hpp`, not in the sibling `core/x86_codegen_output.hpp`. The sibling header currently exposes only text-buffer helpers for generic asm text inspection, while this `.cpp` implements core `X86Codegen*` methods declared elsewhere.

Essential surfaces:

```cpp
struct X86CodegenOutput {
  X86CodegenState* owner = nullptr;
  void emit_instr_imm_reg(const char*, std::int64_t, const char*);
  void emit_instr_reg_rbp(const char*, const char*, std::int64_t);
  void emit_instr_rbp_reg(const char*, std::int64_t, const char*);
  void emit_instr_rbp(const char*, std::int64_t);
  void emit_instr_mem_reg(const char*, std::int64_t, const char*, const char*);
};
```

Contract: these helpers do not emit machine code objects or structured IR. They only stringify AT&T-syntax instructions and append them to `owner->asm_lines`. A null `owner` makes them no-op.

```cpp
struct X86CodegenState {
  X86CodegenOutput out;
  X86CodegenRegCache reg_cache;
  std::vector<std::string> asm_lines;
  std::unordered_map<std::uint32_t, StackSlot> slots;
  std::unordered_map<std::uint32_t, std::uint8_t> reg_assignment_indices;
  ...
};
```

Contract: this state object is the hidden data plane for most helper behavior. Copy/move construction must rebind `out.owner` back to the new state instance.

```cpp
struct X86Codegen {
  X86CodegenState state;
  std::vector<EightbyteClass> func_ret_classes;
  std::vector<EightbyteClass> call_ret_classes;
  ...
  void operand_to_reg(const Operand&, const char* reg);
  void emit_call_store_result_impl(const Value&, IrType);
  void emit_x86_atomic_op_loop(IrType, const char* op);
};
```

Contract: these helpers assume the rest of codegen has already populated stack slots, register assignments, ABI classification vectors, and active return-type state.

At the end of the file, the `core` namespace exposes a separate mini-surface:

```cpp
class AssemblyTextBuffer {
 public:
  void append_line(std::string_view line);
  void append_raw(std::string_view text);
};
```

plus `asm_text_references_symbol(...)` and `asm_text_defines_symbol(...)`, which do token-boundary scanning over rendered asm text.

## Dependency direction and hidden inputs

- Upward dependency: most definitions here are leaf implementations for declarations in `x86_codegen.hpp`.
- Sideways dependency: callers across `memory.cpp`, call lowering, scalar lowering, prologue, return lowering, and other x86 codegen units depend on helpers like `phys_reg_name`, `x86_arg_reg_name`, and `operand_to_reg`.
- Hidden inputs:
  - `state.slots` determines whether a value can be loaded/stored from the stack
  - `state.reg_assignment_indices` decides whether a value lives in an allocated physical register
  - `state.allocas` and `state.over_aligned_allocas` change address materialization behavior
  - `func_ret_classes` and `call_ret_classes` drive ABI-specific return packing/unpacking, especially mixed integer/SSE `i128`
  - `pic_mode`, `got_addr_symbols`, and f128 tracking maps influence later codegen stages even though this file only updates/query them
- Output direction: all instruction helpers ultimately write strings into `state.asm_lines`; no structured emitter abstraction sits between helper logic and final textual assembly.

## Responsibility buckets

### 1. Register/ABI lookup helpers

- argument-register tables for GP and FP arguments
- width conversion helpers (`reg_name_to_32`, internal 64/16/8 mappers)
- physical-register name lookup from allocator indices
- policy bit `x86_allow_struct_split_reg_stack()`, currently hardcoded `false`

These helpers are low-level ABI facts but currently live beside state mutation and lowering logic.

### 2. Output formatting helpers

- `append_asm_line`
- `X86CodegenOutput::emit_instr_*`
- `X86CodegenState::emit`

These are thin string builders for common instruction shapes, mostly stack-slot and immediate/register forms.

### 3. Execution/cache/state helpers

- accumulator cache invalidation and update
- slot lookup and `SlotAddr` resolution
- f128 provenance and constant-word tracking
- GOT-address requirement tracking
- owner rebinding in copy/move operations

This is the real persistent bookkeeping layer for the x86 code generator.

### 4. Operand/result movement

- immediate, register-assigned, stack-slot, and alloca-address materialization into GP regs
- paired `rax`/`rdx` handling for `i128`
- storing accumulator results back to slots
- return-value shuffles between GP regs, XMM regs, and x87 stack paths

This is not pure output logic; it is backend semantic lowering support.

### 5. Type-width instruction selection

- register narrowing by `IrType`
- load/store mnemonic choice
- instruction suffix selection

This is a small policy table used by multiple emitters.

### 6. Generic atomic CAS loop emission

- emits a `lock cmpxchg*` retry loop for a narrow set of RMW ops (`sub`, `and`, `or`, `xor`, `nand`)
- generates local labels using a process-global incrementing counter

This is semantic instruction synthesis, not just output buffering.

### 7. Generic asm text utilities

- append raw or line-oriented text into a separate reusable buffer
- check whether rendered asm text references or defines a symbol with token-boundary awareness

This is the only part that matches the sibling public header.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- zero immediate to GP register uses xor-zeroing instead of `mov`
- operand loads prefer assigned physical registers before stack slots
- direct slot stores update the accumulator cache instead of forcing a reload story later
- typed load/store selector tables avoid repeated branching in callers

### Compatibility paths

- mixed `i128` return-class shuffles support SysV-style integer/SSE pair combinations
- `F32`, `F64`, and `F128` returns bridge among GP registers, XMM registers, and the x87 stack
- over-aligned allocas are handled by rounding a computed frame address up to alignment
- `x86_allow_struct_split_reg_stack()` is an explicit conservative compatibility switch, currently rejecting split reg/stack struct passing

### Overfit pressure

- the file mixes state management, ABI tables, return conventions, x87 quirks, and atomics in one place because they all touch `state`
- `emit_call_store_result_impl` contains ABI-shape branching for specific `i128` class combinations and special `F128` handling; this is the kind of routine that tends to accumulate one-more-case growth
- stringly instruction emission makes local special cases cheap to add and hard to audit structurally
- duplicate/near-duplicate helpers exist elsewhere in the x86 backend (`mod.cpp`, shared support), which suggests ownership drift rather than a single stable seam

## Rebuild ownership guidance

In a rebuild, this file should own only a narrow output/state support seam: small asm-text append utilities, lightweight register-format helpers, and state rebinding/cache helpers that are genuinely shared.

It should not own semantic lowering behavior such as operand materialization policy, ABI return shuffles, atomic retry-loop synthesis, or type-driven instruction selection tables when those decisions belong in dedicated lowering or ABI modules.
