# `globals.cpp` Extraction

## Purpose And Current Responsibility

This file is a thin x86 codegen helper layer for global and label addressing. Its live responsibility is not policy or lowering selection; it emits a few concrete assembly forms into `X86Codegen::state` and funnels results through the class-wide `%rax` result convention.

It also contains three anonymous-namespace recognizers for tiny BIR module shapes. In this translation unit they are only executable evidence of prior specialization pressure, not part of the active emission surface.

## Important APIs And Contract Surfaces

The public surface exposed through the directory index header is small and imperative:

```cpp
void emit_global_addr_impl(const Value& dest, const std::string& name);
void emit_tls_global_addr_impl(const Value& dest, const std::string& name);
void emit_global_addr_absolute_impl(const Value& dest, const std::string& name);
void emit_global_load_rip_rel_impl(const Value& dest, const std::string& sym, IrType ty);
void emit_global_store_rip_rel_impl(const Operand& val, const std::string& sym, IrType ty);
void emit_label_addr_impl(const Value& dest, const std::string& label);
```

Common contract:

- emission target is `this->state.emit(...)`
- address and scalar load results are first materialized in `%rax` or `%eax`
- final destination writeback is delegated through `emit_store_result_impl(dest)`
- stores preload the source operand through `emit_load_operand_impl(val)`

The only type-sensitive local decision is the destination register for RIP-relative loads:

```cpp
const char* global_load_dest_reg(IrType ty);
```

Current mapping is narrow: `U32` and `F32` use `%eax`; everything else falls back to `%rax`.

## Dependency Direction And Hidden Inputs

Dependency direction is inward to `X86Codegen`; this file owns almost no state itself.

- Depends on `X86Codegen::state.pic_mode` to choose TLS addressing form.
- Depends on `X86Codegen::state.needs_got_for_addr(name)` to choose GOT-vs-direct global addressing.
- Depends on class helpers `mov_load_for_type`, `mov_store_for_type`, `reg_for_type`, `operand_to_rax`, and `store_rax_to`.
- Depends on assembler syntax policy already encoded elsewhere; this file only concatenates textual instructions.
- Includes BIR types solely for the anonymous-namespace recognizers.

Hidden input pattern: callers do not pass relocation mode, PIC mode, or register-allocation context explicitly. Those choices are read implicitly from `this->state` and from helper conventions that treat `%rax` as the staging register.

## Responsibility Buckets

### 1. Global And Label Address Materialization

- `emit_global_addr_impl`: RIP-relative address, with GOT indirection when the state marks the symbol as requiring it.
- `emit_tls_global_addr_impl`: TLS address materialization, split by PIC/non-PIC path.
- `emit_global_addr_absolute_impl`: absolute symbol address into `%rax`.
- `emit_label_addr_impl`: local label address via `leaq`.

### 2. RIP-Relative Scalar Data Access

- `emit_global_load_rip_rel_impl`: typed load from `sym(%rip)`.
- `emit_global_store_rip_rel_impl`: typed store to `sym(%rip)`.

These helpers are assembly renderers, not semantic validators. They assume the symbol, type, and relocation mode are already resolved correctly upstream.

### 3. Local Result/Operand Adapters

- `emit_store_result_impl` is just the file-local name for `store_rax_to`.
- `emit_load_operand_impl` is just the file-local name for `operand_to_rax`.

This is convenience wrapping, not ownership of value movement policy.

### 4. Dormant Shape Recognizers

The anonymous-namespace structs and `parse_minimal_*` functions inspect a full `bir::Module` and accept only tiny one-function/one-global shapes. They classify:

- one internal scalar global load
- one extern scalar global load
- one extern global load at fixed byte offset `4`

These routines are not wired into the live emission entry points in this file. They represent compatibility or test-shape pressure, not core global emission behavior.

## Fast Paths, Compatibility Paths, And Overfit Pressure

- Core lowering:
  - global address emission
  - TLS address emission
  - RIP-relative global load/store emission
  - label address emission
- Optional fast path:
  - none clearly isolated here; the file mostly emits direct forms after upstream selection
- Legacy compatibility:
  - GOT-vs-direct addressing switch via `needs_got_for_addr`
  - PIC-vs-non-PIC TLS addressing split
- Overfit pressure to reject in rebuild:
  - the `parse_minimal_*` recognizers hard-code exact module sizes, exact block layout, exact return shape, exact type `I32`, and even a single fixed array offset
  - these are testcase-shaped acceptance filters, not general global-lowering interfaces

## Rebuild Ownership Boundary

This file should own only the final x86 rendering helpers for already-resolved global and label address/data operations, plus any minimal type-to-instruction selection needed to render them.

It should not own BIR whole-module pattern recognition, testcase-shaped slice parsing, relocation-policy discovery beyond explicit inputs, or broad `%rax` result-convention indirection if that convention can be centralized elsewhere.
